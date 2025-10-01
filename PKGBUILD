_pkgbase=sbae5-color
pkgname=${_pkgbase}-dkms
pkgver=0.3
pkgrel=1
pkgdesc='Sound Blaster AE-5 and AE-5plus LED driver'
arch=(any)
depends=(dkms)
arch=(any)
source=(
    Makefile
    sbae5_color.c
    dkms.conf
    sbae5-color.conf
    led_data.h
    50-sbae-color.rules
)
sha256sums=('SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP')

package() {
    install -Dm644 dkms.conf -t "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/
    install -Dm644 sbae5-color.conf -t "${pkgdir}"/usr/lib/modules-load.d/
    install -Dm644 sbae5_color.c led_data.h Makefile -t "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/
    install -Dm644 50-sbae-color.rules -t "${pkgdir}"/etc/udev/rules.d/
}
