FROM node:12.10-buster

MAINTAINER Rafael Ferreira da Silva <rafsilva@isi.edu>

USER root

# add wrench user
RUN useradd --gid 100 --uid 550 --create-home --password wrench wrench

#################################################
# WRENCH's user
#################################################
USER wrench

RUN mkdir -p /home/wrench/dashboard
WORKDIR /home/wrench/dashboard

# dashboard application files
COPY package.json /home/wrench/dashboard
COPY app.js /home/wrench/dashboard
COPY views /home/wrench/dashboard/views
COPY public /home/wrench/dashboard/public
COPY scripts /home/wrench/dashboard/scripts

RUN npm install

EXPOSE 3000

CMD ["node", "app.js"]

# CMD node parser.js ./test_data/data.json ./test_data/energy.json
