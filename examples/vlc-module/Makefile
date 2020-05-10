CONTIKI_PROJECT = vlc
all: $(CONTIKI_PROJECT)

# include sdlib
MODULES_REL += ./sdlib/vip
MODULES_REL += ./sdlib/configure_cooja

# Include CoAP resources
MODULES_REL += ./resources

CONTIKI = ../..

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
include $(CONTIKI)/Makefile.include