
docker pull wrenchproject/wrench-dashboard
docker images -q wrenchproject/wrench-dashboard > images.txt
imageID=$(head -n 1 images.txt)
docker create $imageID
docker ps -aq > containers.txt
containerID=$(head -n 1 containers.txt)
docker cp $1 $containerID:/app/test_data/data.json
docker container start $containerID
mkdir -p public
docker cp $containerID:/app/index.html .
docker cp $containerID:/app/public/. public
rm images.txt
rm containers.txt