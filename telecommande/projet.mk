PROJET_VERSION = 1.0
PROJET_SITE = $(TOPDIR)/package/projet/driver
PROJET_SITE_METHOD = local
PROJET_LICENSE = GPLv3+

PROJET_DEPENDENCIES = linux

define PROJET_BUILD_CMDS
	$(MAKE) -C $(LINUX_DIR) $(LINUX_MAKE_FLAGS) M=$(@D)	
endef

define PROJET_INSTALL_TARGET_CMDS
	$(MAKE) -C $(LINUX_DIR) $(LINUX_MAKE_FLAGS) M=$(@D) modules_install
endef

$(eval $(generic-package))


