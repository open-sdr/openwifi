################################################################################
#
# openwifi
#
################################################################################

OPENWIFI_VERSION = local
OPENWIFI_SITE = $(BR2_EXTERNAL_OPENWIFI_PATH)/..
OPENWIFI_SITE_METHOD = local
OPENWIFI_LICENSE = AGPL-3.0-or-later
OPENWIFI_LICENSE_FILES = LICENSE
OPENWIFI_DEPENDENCIES = dhcp libnl libpcap
OPENWIFI_MODULE_SUBDIRS = driver driver/side_ch

# The local package is the repository root. Do not recursively copy Buildroot,
# previous output trees, or the external tree into Buildroot's private source
# directory.
OPENWIFI_OVERRIDE_SRCDIR_RSYNC_EXCLUSIONS = \
	--exclude buildroot \
	--exclude buildroot-external \
	--exclude output \
	--exclude dl

OPENWIFI_GIT_REV = $(shell git -C $(BR2_EXTERNAL_OPENWIFI_PATH)/.. rev-parse --short=7 HEAD 2>/dev/null || echo 0000000)

define OPENWIFI_GENERATE_DRIVER_HEADERS
	printf '%s\n' '#define USE_NEW_RX_INTERRUPT 1' > $(@D)/driver/pre_def.h
	printf '%s\n' '#define GIT_REV 0x$(OPENWIFI_GIT_REV)' > $(@D)/driver/git_rev.h
endef
OPENWIFI_POST_RSYNC_HOOKS += OPENWIFI_GENERATE_DRIVER_HEADERS

define OPENWIFI_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) \
		$(MAKE) -C $(@D)/user_space/sdrctl_src
	$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) \
		$(MAKE) -C $(@D)/user_space/side_ch_ctl_src
	$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) \
		$(MAKE) -C $(@D)/user_space/inject_80211
endef

define OPENWIFI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/root/openwifi
	cp -a $(@D)/user_space/. $(TARGET_DIR)/root/openwifi/
	$(INSTALL) -m 0755 $(@D)/user_space/sdrctl_src/sdrctl \
		$(TARGET_DIR)/root/openwifi/sdrctl
	$(INSTALL) -m 0755 $(@D)/user_space/side_ch_ctl_src/side_ch_ctl \
		$(TARGET_DIR)/root/openwifi/side_ch_ctl
	$(INSTALL) -m 0755 $(@D)/user_space/inject_80211/inject_80211 \
		$(TARGET_DIR)/root/openwifi/inject_80211/inject_80211
	$(INSTALL) -m 0755 $(@D)/user_space/inject_80211/analyze_80211 \
		$(TARGET_DIR)/root/openwifi/inject_80211/analyze_80211
	$(INSTALL) -m 0644 $(@D)/user_space/dhcpd.conf \
		$(TARGET_DIR)/etc/dhcp/dhcpd.conf
	find $(TARGET_DIR)/root/openwifi -name '*.sh' -exec chmod 0755 {} \;
	$(INSTALL) -D -m 0755 \
		$(BR2_EXTERNAL_OPENWIFI_PATH)/board/common/openwifi-start \
		$(TARGET_DIR)/usr/bin/openwifi-start
	printf '%s\n' unknown > $(TARGET_DIR)/etc/openwifi-board
	$(INSTALL) -D -m 0755 \
		$(BR2_EXTERNAL_OPENWIFI_PATH)/board/common/openwifi-board-name \
		$(TARGET_DIR)/usr/bin/openwifi-board-name
	$(INSTALL) -D -m 0755 \
		$(BR2_EXTERNAL_OPENWIFI_PATH)/board/common/S02openwifi-board \
		$(TARGET_DIR)/etc/init.d/S02openwifi-board
	$(INSTALL) -D -m 0755 \
		$(BR2_EXTERNAL_OPENWIFI_PATH)/board/common/openwifi-fw-update \
		$(TARGET_DIR)/usr/sbin/openwifi-fw-update
endef

# Keep module copies beside wgd.sh because the upstream reload script uses
# insmod on modules in its selected directory.
define OPENWIFI_INSTALL_LOCAL_MODULE_COPIES
	find $(@D)/driver -name '*.ko' -exec cp -f {} \
		$(TARGET_DIR)/root/openwifi/ \;
	$(SED) 's@insmod ad9361_drv.ko@modprobe ad9361 2>/dev/null || true@' \
		$(TARGET_DIR)/root/openwifi/wgd.sh
	$(SED) 's@insmod xilinx_dma.ko@modprobe xilinx_dma 2>/dev/null || true@' \
		$(TARGET_DIR)/root/openwifi/wgd.sh
	$(SED) 's|service dhcpcd stop|true # no dhcpcd in Buildroot|' \
		$(TARGET_DIR)/root/openwifi/wgd.sh
	$(SED) 's|killall webfsd|killall httpd|' \
		$(TARGET_DIR)/root/openwifi/fosdem.sh \
		$(TARGET_DIR)/root/openwifi/fosdem-11ag.sh
	$(SED) 's|webfsd -F -p 80 -f index.html|busybox httpd -f -p 80|' \
		$(TARGET_DIR)/root/openwifi/fosdem.sh \
		$(TARGET_DIR)/root/openwifi/fosdem-11ag.sh
	$(SED) 's|service isc-dhcp-server restart|/etc/init.d/isc-dhcp-server restart|' \
		$(TARGET_DIR)/root/openwifi/fosdem.sh \
		$(TARGET_DIR)/root/openwifi/fosdem-11ag.sh
	$(SED) 's|sudo ||g' \
		$(TARGET_DIR)/root/openwifi/monitor_ch.sh \
		$(TARGET_DIR)/root/openwifi/sdr-ad-hoc-up.sh \
		$(TARGET_DIR)/root/openwifi/sdr-ad-hoc-join.sh \
		$(TARGET_DIR)/root/openwifi/nic_back_to_normal.sh
	ln -snf /boot/system_top.bit.bin \
		$(TARGET_DIR)/root/openwifi/system_top.bit.bin
endef

$(eval $(kernel-module))
OPENWIFI_POST_INSTALL_TARGET_HOOKS += OPENWIFI_INSTALL_LOCAL_MODULE_COPIES
$(eval $(generic-package))
