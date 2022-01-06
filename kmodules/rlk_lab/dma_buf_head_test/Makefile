all:
	gcc  -Wall -O2 -g  -I.  dmabuf_import.c ipcsocket.c dmabuf_utils.c   -o dmabuf_import
	gcc  -Wall -O2 -g  -I. dmabuf_export.c ipcsocket.c dmabuf_utils.c   -o dmabuf_export
clean:
	rm dmabuf_import
	rm dmabuf_export
