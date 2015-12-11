LIGHTS_VERSION = 1.0
LIGHTS_SITE = $(TOPDIR)/package/lights/driver
LIGHTS_SITE_METHOD = local
LIGHTS_LICENSE = GPLv3+
LIGHTS_DEPENDENCIES = linux

define LIGHTS_BUILD_CMDS
	$(MAKE) -C $(LINUX_DIR) $(LINUX_MAKE_FLAGS) M=$(@D)
endef

define LIGHTS_INSTALL_TARGET_CMDS
	$(MAKE) -C $(LINUX_DIR) $(LINUX_MAKE_FLAGS) M=$(@D) modules_install
endef 
 
$(eval $(generic-package)) 