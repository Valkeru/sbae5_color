_pkgbase=sbae5_color
pkgname=${_pkgbase}-dkms
pkgver=0.1
pkgrel=1
pkgdesc='Sound Blaster AE-5 plus backlight control module'
arch=(any)
depends=(dkms)
arch=(any)
source=(
    Makefile
    sbae5_color.c
    dkms.conf
)
sha256sums=('SKIP' 'SKIP' 'SKIP')

package() {
    install -Dm644 dkms.conf "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/dkms.conf
    install -Dm644 sbae5_color.c Makefile -t "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/
}
