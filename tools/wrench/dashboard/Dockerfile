FROM node:7
WORKDIR /app
COPY package.json /app
RUN npm install
COPY . /app
CMD node parser.js ./test_data/data.json ./test_data/energy.json
