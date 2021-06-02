# docker build --no-cache --force-rm -t obmc-webapp-base -f Dockerfile.base .
FROM obmc-webapp-base

RUN mkdir -p /source/build
RUN mkdir -p /usr/share/www
RUN mkdir -p /etc/lighttpd.d
RUN mkdir -p /home/root
RUN mkdir -p /root/.ssh/
RUN mkdir -p /etc/ssl/certs/https/

ADD .ssh/ /root/.ssh/
ADD tests/conf/bmcweb_persistent_data.json /home/root/

CMD ["/bin/bash", "-c", "mkdir -p /tmp/bmcweb_metadata && ln -s /home/root/bmcweb_persistent_data.json /tmp/bmcweb_metadata/sessions.json && cd /source/build && meson .. && ninja && ninja test && /bin/bash"]
