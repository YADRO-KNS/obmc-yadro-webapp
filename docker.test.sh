#!/bin/bash -eu

function run_container() {
  DOCKER_IMAGE="obmc-webapp-server"

  CID=
  function on_exit {
    # Completely remove container after usage
    [[ -z "${CID}" ]] || docker rm --force --volumes "${CID}" > /dev/null
  }
  trap on_exit EXIT

  SOURCE_DIR="${SOURCE_DIR:-$(realpath $(dirname ${BASH_SOURCE[0]}))}"
  [[ -d ${DLCACHE_DIR:-} ]] && DLCACHE_OPT="--volume ${DLCACHE_DIR}:${DLCACHE_DIR}:Z" || DLCACHE_OPT=
  [[ -t 1 ]] && PASSTTY_OPT="--tty" || PASSTTY_OPT=

  CID=$(docker create \
              ${PASSTTY_OPT} \
              --interactive \
              --env HOME=/source/build/ \
              --env "SSH_AUTH_SOCK=/ssh-agent" \
              --tmpfs /tmp \
              --tmpfs /run \
              --tmpfs /run/lock \
              --volume "${SOURCE_DIR}":/source:Z \
              --volume "${SSH_AUTH_SOCK}":/ssh-agent \
              --volume /sys/fs/cgroup:/sys/fs/cgroup:ro \
              --volume /run/systemd/journal/socket:/run/systemd/journal/socket \
              --publish 18081:18081 \
              --publish 30022:22 \
              --privileged \
              ${DLCACHE_OPT} \
              --workdir "${SOURCE_DIR}" \
              "${DOCKER_IMAGE}" \
              "$@")

  docker start --attach --interactive ${CID}
}

function build_docker_images() {
  echo "Build 'obmc-webapp-base' image..."
  docker build --no-cache --force-rm -t obmc-webapp-base -f Dockerfile.base .
  echo "Build 'obmc-webapp-server' image..."
  docker build --no-cache --force-rm -t obmc-webapp-server -f Dockerfile .
  echo "Building obmc-webapp docker images are successful."
}

COMMAND=${1:-"run"}

case "${COMMAND}" in
  run)
    mkdir -p build
    run_container "$@"
    exit
    ;;

  build)
    build_docker_images
    exit
    ;;
  help)
    echo "Usage: ${0} run|build|help"
    exit
    ;;
  *)
    echo "Unknown command: ${COMMAND}" >&1
    exit
    ;;

esac
