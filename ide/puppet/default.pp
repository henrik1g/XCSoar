class apt {
  exec { 'apt-get update':
    command => '/usr/bin/apt-get update',
  }
}

class libcurl {
  package { "libcurl4-openssl-dev":
    ensure => present,
  }
}

class libsdl {
  package { "libsdl1.2-dev":
    ensure => present,
  }

  package { "libsdl-image1.2-dev":
    ensure => present,
  }

  package { "libsdl-ttf2.0-dev":
    ensure => present,
  }

  # Workaround for broken libsdl-ttf2.0-dev package on Ubuntu 12.04
  exec { 'SDL_ttf.pc':
    command => '/bin/sed \'s/SDL_image/SDL_ttf/g\' /usr/lib/i386-linux-gnu/pkgconfig/SDL_image.pc > /usr/lib/i386-linux-gnu/pkgconfig/SDL_ttf.pc',
    require => [Package['libsdl-ttf2.0-dev'], Package['libsdl-image1.2-dev']],
    creates => '/usr/lib/i386-linux-gnu/pkgconfig/SDL_ttf.pc',
  }
}

class libboost {
  package { "libboost-dev":
    ensure => present,
  }
}

class gxx {
  package { "g++":
    ensure => present,
  }
}

class mingw-w64 {
  package { "gcc-mingw-w64":
    ensure => present,
  }

  package { "g++-mingw-w64":
    ensure => present,
  }
}

class mingw32ce {
  exec { 'mingw32ce-mk-2012-03-13-i386.tar.bz2':
    command => '/usr/bin/wget http://max.kellermann.name/download/xcsoar/devel/cegcc/mingw32ce-mk-2012-03-13-i386.tar.bz2',
    cwd => '/home/vagrant/',
    creates => '/home/vagrant/mingw32ce-mk-2012-03-13-i386.tar.bz2',
    user => 'vagrant',
  }

  exec { 'mingw32ce-mk-2012-03-13-i386':
    command => '/bin/tar -xf mingw32ce-mk-2012-03-13-i386.tar.bz2',
    cwd => '/home/vagrant/',
    creates => '/home/vagrant/mingw32ce-mk-2012-03-13-i386/bin/arm-mingw32ce-g++',
    user => 'vagrant',
    require => Exec['mingw32ce-mk-2012-03-13-i386.tar.bz2'],
  }

  exec { 'mingw32ce-to-path':
    command => '/bin/echo PATH="/home/vagrant/mingw32ce-mk-2012-03-13-i386/bin:\$PATH" >> .profile',
    cwd => '/home/vagrant/',
    require => Exec['mingw32ce-mk-2012-03-13-i386'],
    unless => '/bin/grep mingw32ce .profile',
  }

  # Workaround for library version mismatch
  exec { 'libmpfr-symlink':
    command => '/bin/ln -s libmpfr.so.4 libmpfr.so.1',
    cwd => '/usr/lib/i386-linux-gnu/',
    creates => '/usr/lib/i386-linux-gnu/libmpfr.so.1',
  }

  exec { 'libgmp-symlink':
    command => '/bin/ln -s libgmp.so.10 libgmp.so.3',
    cwd => '/usr/lib/i386-linux-gnu/',
    creates => '/usr/lib/i386-linux-gnu/libgmp.so.3',
  }
}

class make {
  package { "make":
    ensure => present,
  }
}

class ccache {
  package { "ccache":
    ensure => present,
  }
}

class gettext {
  package { "gettext":
    ensure => present,
  }
}

class xsltproc {
  package { "xsltproc":
    ensure => present,
  }
}

class rsvg {
  package { "librsvg2-bin":
    ensure => present,
  }
}

class imagemagick {
  package { "imagemagick":
    ensure => present,
  }
}

# Stages
stage { ['pre', 'post']: }
Stage['pre'] -> Stage['main'] -> Stage['post']

# Update apt package index
class {'apt': stage => 'pre' }

# Libraries
class {'libcurl': }
class {'libsdl': }
class {'libboost': }

# Toolchain
class {'make': }
class {'gxx': }
class {'ccache': }

# MinGW Toolchain
class {'mingw-w64': }

# CeGCC Toolchain
class {'mingw32ce': }

# Resource tools
class {'gettext': }
class {'xsltproc': }
class {'rsvg': }
class {'imagemagick': }
