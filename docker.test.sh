#!/bin/bash -exu

# docker build --no-cache --force-rm -t obmc-webapp-server -f Dockerfile .

DOCKER_IMAGE="obmc-webapp-server"

CID=
function on_exit {
  # Completely remove container after usage
  [[ -z "${CID}" ]] || docker rm --force --volumes ${CID} > /dev/null
}
trap on_exit EXIT

SOURCE_DIR="${SOURCE_DIR:-$(realpath $(dirname ${BASH_SOURCE[0]}))}"
[[ -d ${DLCACHE_DIR:-} ]] && DLCACHE_OPT="--volume ${DLCACHE_DIR}:${DLCACHE_DIR}:Z" || DLCACHE_OPT=
[[ -t 1 ]] && PASSTTY_OPT="--tty" || PASSTTY_OPT=

CID=$(docker create \
             ${PASSTTY_OPT} \
             --interactive \
             --env HOME=/source/build/ \
             --env "$SSH_AUTH_SOCK:/ssh-agent" \
             --env "SSH_AUTH_SOCK=/ssh-agent" \
             --volume ${SOURCE_DIR}:/source:Z \
             --tmpfs /tmp --tmpfs /run --tmpfs /run/lock \
             --privileged -v /sys/fs/cgroup:/sys/fs/cgroup:ro \
             -v /run/systemd/journal/socket:/run/systemd/journal/socket \
             -p 18081:18081 \
             -p 30022:22 \
             ${DLCACHE_OPT} \
             --workdir ${SOURCE_DIR} \
             ${DOCKER_IMAGE} \
             $@)

docker start --attach --interactive ${CID}
