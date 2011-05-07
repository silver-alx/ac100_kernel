include $(RT28xx_DIR)/os/linux/config.mk


#ifdef CONFIG_STA_SUPPORT
ifeq ($(RT28xx_MODE), STA)
MOD_NAME = rt$(CHIPSET)sta
DAT_PATH = /etc/Wireless/RT$(CHIPSET)STA
DAT_FILE_NAME = RT$(CHIPSET)STA.dat
endif
#endif // CONFIG_STA_SUPPORT //


obj-m := $(MOD_NAME).o


#ifdef CONFIG_STA_SUPPORT
rt$(CHIPSET)sta-objs := \
	../../common/crypt_md5.o\
	../../common/crypt_sha2.o\
	../../common/crypt_hmac.o\
	../../common/crypt_aes.o\
	../../common/crypt_arc4.o\
	../../common/mlme.o\
	../../common/cmm_wep.o\
	../../common/action.o\
	../../common/cmm_data.o\
	../../common/rtmp_init.o\
	../../common/cmm_tkip.o\
	../../common/cmm_aes.o\
	../../common/cmm_sync.o\
	../../common/eeprom.o\
	../../common/cmm_sanity.o\
	../../common/cmm_info.o\
	../../common/cmm_cfg.o\
	../../common/cmm_wpa.o\
	../../common/cmm_wpa_adhoc.o\
	../../common/dfs.o\
	../../common/spectrum.o\
	../../common/rtmp_timer.o\
	../../common/rt_channel.o\
	../../common/cmm_profile.o\
	../../common/cmm_asic.o\
	../../common/cmm_cmd.o\
	../../sta/assoc.o\
	../../sta/auth.o\
	../../sta/auth_rsp.o\
	../../sta/sync.o\
	../../sta/sanity.o\
	../../sta/rtmp_data.o\
	../../sta/connect.o\
	../../sta/wpa.o\
	../../sta/sta_cfg.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
	../../common/rtmp_init_inf.o\
	../../os/linux/rt_profile.o\
	../../os/linux/sta_ioctl.o\
	../../os/linux/rt_linux.o\
	../../os/linux/rt_main_dev.o
endif

#ifdef DOT11_N_SUPPORT
ifeq ($(HAS_DOT11_N_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	../../common/ba_action.o
endif
#endif // DOT11_N_SUPPORT //

#ifdef ETH_CONVERT
ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
rt$(CHIPSET)sta-objs += \
	../../common/cmm_mat.o \
	../../common/cmm_mat_iparp.o \
	../../common/cmm_mat_pppoe.o \
	../../common/cmm_mat_ipv6.o
endif
#endif // ETH_CONVERT //

#ifdef WSC_INCLUDED
ifeq ($(HAS_WSC),y)
rt$(CHIPSET)sta-objs += \
	../../common/wsc.o\
	../../common/wsc_tlv.o\
	../../common/crypt_biginteger.o\
	../../common/crypt_dh.o

ifeq ($(HAS_SAMSUNG_EASY_CONFIG_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	../../sta/sta_auto_provision.o
endif
endif
#endif // WSC_INCLUDED //

ifeq ($(HAS_BLOCK_NET_IF),y)
rt$(CHIPSET)sta-objs += ../../common/netif_block.o
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
rt$(CHIPSET)sta-objs += ../../sta/dls.o
endif

#ifdef DOT11Z_TDLS_SUPPORT
ifeq ($(HAS_DOT11Z_TDLS_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	../../sta/tdls.o\
	../../sta/tdls_ctrl.o\
	../../sta/tdls_tlv.o\
	../../sta/tdls_link_mng.o
endif
#endif // DOT11Z_TDLS_SUPPORT //




#ifdef RT3070
ifeq ($(CHIPSET),2070)
rt$(CHIPSET)sta-objs += \
	../../common/cmm_mac_usb.o\
	../../common/rtusb_io.o\
	../../common/rtusb_bulk.o\
	../../common/rtusb_data.o\
	../../common/cmm_data_usb.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/rtmp_mcu.o\
	../../chips/rt30xx.o\
	../../common/rt_rf.o\
	../../chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
	../../common/rtusb_dev_id.o\
	../../os/linux/rt_usb.o\
	../../os/linux/rt_usb_util.o\
	../../os/linux/usb_main_dev.o
endif
endif

ifeq ($(CHIPSET),3070)
rt$(CHIPSET)sta-objs += \
	../../common/cmm_mac_usb.o\
	../../common/rtusb_io.o\
	../../common/rtusb_bulk.o\
	../../common/rtusb_data.o\
	../../common/cmm_data_usb.o\
	../../common/ee_prom.o\
	../../common/ee_efuse.o\
	../../common/rtmp_mcu.o\
	../../chips/rt30xx.o\
	../../common/rt_rf.o\
	../../chips/rt3070.o

ifeq ($(OSABL),NO)
rt$(CHIPSET)sta-objs += \
	../../common/rtusb_dev_id.o\
	../../os/linux/rt_usb.o\
	../../os/linux/rt_usb_util.o\
	../../os/linux/usb_main_dev.o
endif
endif
#endif // RT3070 //





ifeq ($(HAS_ATE),y)
rt$(CHIPSET)sta-objs += ../../os/linux/rt_ate.o
endif

#ifdef WMM_ACM_SUPPORT
ifeq ($(HAS_WMM_ACM_SUPPORT),y)
rt$(CHIPSET)sta-objs += ../../common/acm_edca.o\
	../../common/acm_comm.o\
	../../common/acm_iocl.o
endif
#endif // WMM_ACM_SUPPORT //


#ifdef WAPI_SUPPORT
ifeq ($(HAS_WAPI_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	../../common/wapi.o
endif
#endif // WAPI_SUPPORT //



#ifdef CRDA_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_CFG80211_SUPPORT),y)
rt$(CHIPSET)sta-objs += \
	../../os/linux/cfg80211.o
endif
endif
#endif // CRDA_SUPPORT //

rt$(CHIPSET)sta-objs += \
	../../common/misc.o

#endif // CONFIG_STA_SUPPORT //


clean:
	rm -f ../../common/*.o
	rm -f ../../common/.*.{cmd,flags,d}
	rm -f ../../os/linux/*.{o,ko,mod.{o,c}}
	rm -f ../../os/linux/.*.{cmd,flags,d}
	rm -fr ../../os/linux/.tmp_versions
#Must clean Module.symvers; or you will suffer symbol version not match
#when OS_ABL = YES.
	rm -f ../../os/linux/Module.symvers
	rm -f ../../chips/*.o
	rm -f ../../chips/.*.{cmd,flags,d}
ifeq ($(RT28xx_MODE),AP)
	rm -f ../../ap/*.o
	rm -f ../../ap/.*.{cmd,flags,d}
else
ifeq ($(RT28xx_MODE),STA)
	rm -f ../../sta/*.o
	rm -f ../../sta/.*.{cmd,flags,d}
else
ifeq ($(RT28xx_MODE),APSTA)
	rm -f ../../ap/*.o
	rm -f ../../ap/.*.{cmd,flags,d}
	rm -f ../../sta/*.o
	rm -f ../../sta/.*.{cmd,flags,d}
endif
endif
endif

install:
	rm -rf $(DAT_PATH)
	$(shell [ ! -f /etc/Wireless ] && mkdir /etc/Wireless)
	mkdir $(DAT_PATH)
	cp $(RT28xx_DIR)/$(DAT_FILE_NAME) $(DAT_PATH)/.
	install -d $(LINUX_SRC_MODULE)
	install -m 644 -c $(addsuffix .ko,$(MOD_NAME)) $(LINUX_SRC_MODULE)
	/sbin/depmod -a ${shell uname -r}

uninstall:
#	rm -rf $(DAT_PATH)
	rm -rf $(addprefix $(LINUX_SRC_MODULE),$(addsuffix .ko,$(MOD_NAME)))
	/sbin/depmod -a ${shell uname -r}
