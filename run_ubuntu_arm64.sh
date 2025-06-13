#!/bin/bash

LROOT=$PWD
JOBCOUNT=${JOBCOUNT=$(nproc)}
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
export INSTALL_PATH=$LROOT/rootfs_ubuntu_arm64/boot/
export INSTALL_MOD_PATH=$LROOT/rootfs_ubuntu_arm64/
export INSTALL_HDR_PATH=$LROOT/rootfs_ubuntu_arm64/usr/

kernel_build=$PWD/rootfs_ubuntu_arm64/usr/src/linux/
rootfs_path=$PWD/rootfs_ubuntu_arm64
rootfs_image=$PWD/rootfs_ubuntu_arm64.ext4

rootfs_size=2048
SMP="-smp 4"

QEMU=qemu-system-aarch64-6.2

rootfs_arg="root=/dev/vda rootfstype=ext4 rw"
kernel_arg="noinintrd nokaslr net.ifnames=0 biosdevname=0"
crash_arg="crashkernel=256M"
dyn_arg="device.dyndbg=+pflmt vfio.dyndbg=+pflmt irq_gic_v3_its.dyndbg=+pflmt iommu.dyndbg=+pflmt irqdomain.dyndbg=+pflmt"
debug_arg="loglevel=8 sched_debug"

if [ $# -lt 1 ]; then
	echo "Usage: $0 [arg]"
	echo "menuconfig: reconfig the kernel"
	echo "build_kernel: build the kernel image."
	echo "build_rootfs: build the rootfs image, need root privilege"
	echo "update_rootfs: update kernel modules for rootfs image, need root privilege."
	echo "run: run ubuntu system."
	echo "run debug: enable gdb debug server."
fi

if [ $# -eq 2 ] && [ $2 == "debug" ]; then
	echo "Enable qemu debug server"
	DBG="-s -S"
	SMP=""
fi

make_kernel_image(){
		echo "start build kernel image..."
		make debian_defconfig
		make -j $JOBCOUNT
}

make_menuconfig(){
               echo "start re-config kernel"
               make debian_defconfig
               make menuconfig
               cp .config $PWD/arch/arm64/configs/debian_defconfig
}

prepare_rootfs(){
		if [ -f $rootfs_image ]; then
			echo "rootfs_ubuntu_arm64.ext4 existing!!!"
			echo "pls run: sudo ./run_ubuntu_arm64.sh update_rootfs."
			echo "or delet the ext4 image and run again!"
			exit 1
		fi

		if [ ! -d $rootfs_path ]; then
			echo "decompressing rootfs..."
			# split -d -b 80m rootfs_ubuntu_arm64.tar.xz -- rootfs_ubuntu_arm64.part 
			cat rootfs_ubuntu_arm64.part0* > rootfs_ubuntu_arm64.tar.xz
			tar -Jxf rootfs_ubuntu_arm64.tar.xz
		fi
}

build_kernel_devel(){
	kernver="$(cat include/config/kernel.release)"
	echo "kernel version: $kernver"

	rm -rf $kernel_build
	mkdir -p $kernel_build
	rm rootfs_ubuntu_arm64/lib/modules/$kernver/build

	cp -a include $kernel_build
	cp Makefile .config Module.symvers System.map vmlinux $kernel_build
	mkdir -p $kernel_build/arch/arm64/
	mkdir -p $kernel_build/arch/arm64/kernel/

	cp -a arch/arm64/include $kernel_build/arch/arm64/
	cp -a arch/arm64/Makefile $kernel_build/arch/arm64/

	# cp from linux-headers-5.15.0-25-generic package
	cp -a rootfs_ubuntu_arm64/usr/src/linux-headers-5.15.0-25-generic/scripts $kernel_build
	cp -a rootfs_ubuntu_arm64/usr/src/linux-headers-5.15.0-25-generic/tools $kernel_build

	cp  scripts/module.lds $kernel_build/scripts/

	# link to /lib/modules/xxx/build
	ln -s /usr/src/linux rootfs_ubuntu_arm64/lib/modules/$kernver/build
}

check_root(){
		if [ "$(id -u)" != "0" ];then
			echo "superuser privileges are required to run"
			echo "sudo ./run_ubuntu_arm64.sh build_rootfs"
			exit 1
		fi
}

update_rootfs(){
		if [ ! -f $rootfs_image ]; then
			echo "rootfs image is not present..., pls run build_rootfs"
		else
			echo "update rootfs ..."

			mkdir -p $rootfs_path
			echo "mount ext4 image into rootfs_ubuntu_arm64"
			mount -t ext4 $rootfs_image $rootfs_path -o loop

			make install
			make modules_install -j $JOBCOUNT
			#make headers_install

			build_kernel_devel

			umount $rootfs_path
			chmod 777 $rootfs_image

			rm -rf $rootfs_path
		fi

}

build_rootfs(){
		if [ ! -f $rootfs_image ]; then
			make install
			make modules_install -j $JOBCOUNT
			# make headers_install

			build_kernel_devel

			echo "making image..."
			dd if=/dev/zero of=$rootfs_image bs=1M count=$rootfs_size
			mkfs.ext4 $rootfs_image
			mkdir -p tmpfs
			echo "copy data into rootfs..."
			mount -t ext4 $rootfs_image tmpfs/ -o loop
			cp -af $rootfs_path/* tmpfs/
			umount tmpfs
			chmod 777 $rootfs_image

			rm -rf $rootfs_path
		fi

}

# add "nokaslr" into kernel command line or disable CONFIG_RANDOMIZE_BASE
run_qemu_ubuntu(){
		if [ ! -f $LROOT/kmodules ]; then
			mkdir -p $LROOT/kmodules
		fi

		cmd="$QEMU -m 1024 -cpu max,sve=on,sve256=on -M virt,gic-version=3,its=on,iommu=smmuv3,mte=on\
			-nographic $SMP -kernel arch/arm64/boot/Image \
			-append \"$kernel_arg $debug_arg $rootfs_arg $crash_arg $dyn_arg\"\
			-drive if=none,file=$rootfs_image,id=hd0\
			-device virtio-blk-device,drive=hd0\
			--fsdev local,id=kmod_dev,path=./kmodules,security_model=none\
			-device virtio-9p-pci,fsdev=kmod_dev,mount_tag=kmod_mount\
			-device virtio-net-pci,netdev=mynet -netdev user,id=mynet\
			-device e1000e\
			$DBG"
		echo "running:"
		echo $cmd
		eval $cmd
}

case $1 in
	build_kernel)
		make_kernel_image
		#prepare_rootfs
		#build_rootfs
		;;
	
	menuconfig)
		make_menuconfig
		;;

	build_rootfs)
		#make_kernel_image
		check_root
		prepare_rootfs
		build_rootfs
		;;

	update_rootfs)
		update_rootfs
		;;
	run)

		if [ ! -f $LROOT/arch/arm64/boot/Image ]; then
			echo "canot find kernel image, pls run build_kernel command firstly!!"
			exit 1
		fi

		if [ ! -f $rootfs_image ]; then
			echo "canot find rootfs image, pls run build_rootfs command firstly!!"
			exit 1
		fi

		#prepare_rootfs
		#build_rootfs
		run_qemu_ubuntu
		;;
esac

