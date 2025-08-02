# Maintainer: aserdevyt <aserdevyt@outlook.com>
pkgname=ash-shell-git
pkgver=1.0.r0.g$(date +%s)
pkgrel=1
pkgdesc="A modern, secure, feature-rich Linux shell written in C"
arch=('x86_64')
url="https://github.com/aserdevyt/ash-shell"
license=('MIT')
depends=('readline')
makedepends=('git' 'cmake' 'gcc')
source=("git+https://github.com/aserdevyt/ash-shell.git")
md5sums=('SKIP')

build() {
  cd "$srcdir/ash-shell"
  mkdir -p build
  cd build
  cmake ..
  make
}

package() {
  cd "$srcdir/ash-shell/build"
  install -Dm755 ash "$pkgdir/usr/bin/ash"
}