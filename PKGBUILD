# PKGBUILD for zerotier-status-daemon
pkgname=zerotier-status-daemon
pkgver=1.0.7
pkgrel=1
pkgdesc="A native Qt C++ system tray status indicator for ZeroTier"
arch=('x86_64')
license=('GPL3')
depends=('qt6-base' 'zerotier-one')
makedepends=('cmake')
source=('main.cpp' 'CMakeLists.txt' 'zerotier-status-daemon.desktop' '49-zerotier.rules')
sha256sums=('SKIP' 'SKIP' 'SKIP' 'SKIP')

build() {
    cmake -B build -S "$srcdir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
    install -Dm644 "$srcdir/zerotier-status-daemon.desktop" "$pkgdir/usr/share/applications/zerotier-status-daemon.desktop"
    install -Dm644 "$srcdir/49-zerotier.rules" "$pkgdir/usr/share/polkit-1/rules.d/49-zerotier.rules"
}
