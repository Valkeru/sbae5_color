_pkgbase=sbae5_color
pkgname=${_pkgbase}-dkms
pkgver=0.1
pkgrel=2
pkgdesc='Sound Blaster AE-5 plus LED control module'
arch=(any)
depends=(dkms)
arch=(any)
source=(
    Makefile
    sbae5_color.c
    dkms.conf
    sbae5_color.conf
)
sha256sums=('SKIP' 'SKIP' 'SKIP' 'SKIP')

package() {
    install -Dm644 dkms.conf -t "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/
    install -Dm644 sbae5_color.conf -t "${pkgdir}"/usr/lib/modules-load.d/
    install -Dm644 sbae5_color.c Makefile -t "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/
}
