################################################################################
#
# libad9361-iio
#
################################################################################

LIBAD9361_IIO_VERSION = 0.3
LIBAD9361_IIO_SITE = $(call github,analogdevicesinc,libad9361-iio,v$(LIBAD9361_IIO_VERSION))
LIBAD9361_IIO_INSTALL_STAGING = YES
LIBAD9361_IIO_LICENSE = LGPL-2.1+
LIBAD9361_IIO_LICENSE_FILES = LICENSE
LIBAD9361_IIO_DEPENDENCIES = libiio

# The upstream project enables packaging/documentation opportunistically when
# host tools are present.  Keep the target build limited to the shared library,
# public header and pkg-config metadata.
LIBAD9361_IIO_CONF_OPTS = \
	-DOSX_FRAMEWORK=OFF \
	-DOSX_PACKAGE=OFF \
	-DENABLE_PACKAGING=OFF \
	-DWITH_DOC=OFF \
	-DPYTHON_BINDINGS=OFF \
	-DMATLAB_BINDINGS=OFF

$(eval $(cmake-package))
