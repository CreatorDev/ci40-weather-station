include $(TOPDIR)/rules.mk

PKG_NAME:=weather-station
PKG_VERSION:=1.1.0
PKG_RELEASE:=2

CMAKE_INSTALL:=1

CMAKE_OPTIONS += -DSTAGING_DIR=$(STAGING_DIR)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/weather-station
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:= +awalwm2m +letmecreate +curl +crypto +ssl +glib2
  TITLE:=Weather Station gateway app
endef

define Package/weather-station/description
  Imagination Technologies Weather Station gateway app
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) weather-station/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(call Build/Compile/Default,all)
endef

define Build/Clean
	$(call Build/Compile/Default,clean)
endef

define Package/weather-station/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/bin/data/weather-station
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/data/object_definitions.xml $(1)/usr/bin/data/weather-station
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
