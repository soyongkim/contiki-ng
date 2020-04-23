CONTIKI_PROJECT = coap-example-server
all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1
MAKE_NET = MAKE_NET_NULLNET

# Include CoAP resources
MODULES_REL += ./resources

CONTIKI=../../..

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
include $(CONTIKI)/Makefile.include
