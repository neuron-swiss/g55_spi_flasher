DESCRIPTION = "Spi flash application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://spiflasher.c"
RDEPENDS_${PN} = "libsoc"
DEPENDS = "libsoc"

S = "${WORKDIR}"

do_compile() {
    ${CC} spiflasher.c ${LDFLAGS} -lsoc -o spiflasher
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 spiflasher ${D}${bindir}
}
