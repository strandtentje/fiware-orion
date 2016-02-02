# Building from sources

Refer to build_source.md for information on building on the recommended
distribution (CentOS 6.x.)

This document outlines compilation on Debian Stretch. Using this method is not recommended or supported but may be preferable or informative.

## Debian Stretch (Not officially supported but otherwise a fair idea)

The Orion Context Broker uses the following libraries as build dependencies. Note that some packages are expected to find their origin in the EPEL6 repository, which is not easily available on Debian. This isn't a humongous problem.

### Libraries from Debian Repo

Behind the recommended libraries and versions, the package names and versions that Debian Stretch provides have been noted

* boost: 1.41+ (libboost-all-dev | 1.58)
* libmicrohttpd: 0.9.48 (libmicrohttpd-dev | 0.9.44)
* libcurl: 7.19.7+ (libcurl4-gnutls-dev | 7.47.0-1)

### Libraries for Manual Install

* Mongo Driver: legacy-1.0.7 
* rapidjson: 1.0.2 
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)

### Tools for Manual Install

Drop into a root shell for executing this section. If you are unsure how to do this:
        
        # Ubuntu
        sudo bash


        # Debian
        su

* Install the needed building tools (compiler, etc.).

        apt-get install scons build-essential cmake

* Install the libraries listed above as follows:

        apt-get install libboost-all-dev libmicrohttpd-dev libcurl4-gnutls-dev 

* Install the Mongo Driver from source:

Note we instruct the build script to *not* treat warnings as errors; this legacy driver brings some relatively legacy code that the compiler isn't immediately happy about.

        # Run these as a regular user:
        wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.0.7.tar.gz
        tar xf legacy-1.0.7.tar.gz
        cd mongo-cxx-driver-legacy-1.0.7
        # produce build/linux2/normal/libmongoclient.a:
        scons --disable-warnings-as-errors                             
        # To put binaries and headers in their respective /usr/local-dirs,
        # Run this as root again:
        scons install --prefix=/usr/local  --disable-warnings-as-errors  

* Install rapidjson from sources:

        # Run these as a regular user:
        wget https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz
        tar xf v1.0.2.tar.gz
        # Run this as root again:
        mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include

* Install Google Test/Mock from sources (there are RPM pacakges for this, but they don't seem to be working with the current CMakeLists.txt configuration)

        # Run these as a regular user:
        wget http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        # To put binaries and headers in their respective /usr/local-dirs,
        # Run these as root again:
        make install  
        # just in case... it doesn't hurt :)
        ldconfig      

* Get the code using git:

        # Run as root in case you don't have git
        apt-get install git
        # Run as regular user
        git clone https://github.com/telefonicaid/fiware-orion
        # Navigate to project tree
        cd fiware-orion/
        # Checkout the release (at the time of writing this, 0.27.0)
        git checkout tags/0.27.0

* Alternatively, get a tarball with the code:

        # Download the tarball
        wget https://github.com/telefonicaid/fiware-orion/archive/0.27.0.tar.gz
        # Untar it
        tar xf 0.27.0.tar.gz
        # Navigate to project tree 
        cd fiware-orion-0.27.0/

* Modify the CMakeLists.txt to not yell at older code

1. On Line 62, find the ' -Werror' clause in the baseline compiler flag definition
2. Remove it
3. Pat yourself on the back for living on the edge

* Build the source:

        make

* (Optional but highly recommended) run unit test. Firstly, you have to install MongoDB (as the unit tests rely on mongod running in localhost).

        sudo yum install mongodb-server
        sudo yum  update pcre            # otherwise, mongod crashes in CentOS 6.3
        sudo /etc/init.d/mongod start
        sudo /etc/init.d/mongod status   # to check that mongod is actually running
        make unit_test

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

* Testing

It is highly recommended to run through the test suite thoroughly, refer to the original document (build_source.md) for that.
