# open62541 Docker Build

Official docker container builds are available on [Docker Cloud](https://cloud.docker.com/u/open62541/repository/registry-1.docker.io/open62541/open62541)

The container includes the source code itself under `/opt/open62541` and prebuilt examples in `/opt/open62541/build/bin/examples/`.

You can use this container as a basis for your own application. 

Just starting the docker container will start the `server_ctt` example.

## Build locally

To build the container locally use:

```bash
git clone https://github.com/open62541/open62541
cd open62541
docker build -f tools/docker/Dockerfile .
```
