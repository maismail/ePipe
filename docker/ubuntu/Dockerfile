ARG ubuntu_image=22.04

FROM ubuntu:${ubuntu_image}

ARG boost_version=1.73.0
ARG rondb_version=22.10.3
ARG glibc_version=2.28
ARG arch=x86_64
ARG userid=1000
ARG groupid=1000

RUN echo Setup environmen; \
apt-get update -y; \
apt-get install -y build-essential wget openssl libssl-dev git make cmake

RUN echo Installing Boost; \
BOOST_V=`echo ${boost_version} | sed s/'\.'/'_'/g`; \
wget --progress=bar:force https://boostorg.jfrog.io/artifactory/main/release/${boost_version}/source/boost_${BOOST_V}.tar.gz; \
tar xzf boost_${BOOST_V}.tar.gz; \
cd boost_${BOOST_V}; \
./bootstrap.sh; \
./b2 link=static toolset=gcc cxxstd=14 install; \
cd ..; \
rm -r boost_${BOOST_V}*

# Use master to avoid compilation issues (https://github.com/Tencent/rapidjson/issues/1205)
# will update to new version when this is fixed to a version
RUN echo Installing RapidJson; \
git clone https://github.com/Tencent/rapidjson.git; \
cd rapidjson; \
git checkout master; \
mkdir build; \
cd build; \
cmake ..; \
make install

RUN echo Installing MySQL Cluster; \
cd /usr/local; \
wget --progress=bar:force https://repo.hops.works/master/rondb-${rondb_version}-linux-glibc${glibc_version}-${arch}.tar.gz; \
tar xzf rondb-${rondb_version}-linux-glibc${glibc_version}-${arch}.tar.gz; \
mkdir -p mysql/lib; \
cp -r rondb-${rondb_version}-linux-glibc${glibc_version}-${arch}/include mysql/; \
cp -r rondb-${rondb_version}-linux-glibc${glibc_version}-${arch}/lib/* mysql/lib/; \
rm -r rondb-${rondb_version}-linux-glibc${glibc_version}-${arch}*

RUN groupadd epipe --gid ${groupid}; \
useradd -ms /bin/bash epipe --uid ${userid} --gid ${groupid}

USER epipe
