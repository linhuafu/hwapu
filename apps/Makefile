include $(TOPDIR)/config.mk

SUBDIRS = desktop menu pc-connector alarm backup jump-page book calculator file-management format help music radio recorder upgrader_show upgrader read_flash_to_xml write_serial_num plextalk_version write_xml_to_flash
#SUBDIRS = desktop menu pc-connector alarm backup jump-page book calculator file-management format help music radio recorder upgrader_show upgrader read_flash_to_xml write_serial_num
.PHONY :$(SUBDIRS)
all: .depend $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ all

.depend dep:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir .depend ; done

clean:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir clean ; done
