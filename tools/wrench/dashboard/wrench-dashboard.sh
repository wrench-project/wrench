
# docker pull wrenchproject/wrench-dashboard
# docker images -q wrenchproject/wrench-dashboard > images.txt
docker pull spenceralbrecht/wrench-dashboard
docker images -q spenceralbrecht/wrench-dashboard > images.txt

imageID=$(head -n 1 images.txt)
docker create $imageID
docker ps -aq > containers.txt
containerID=$(head -n 1 containers.txt)

if [ "$#" -eq 0 ]
then
    echo "
    ERROR: Please provide the file path to the task JSON file after the script name"
    exit
fi

docker cp $1 $containerID:/app/test_data/data.json

# Check if an energy file was provided as input
if [ "$#" -eq 2 ]
then
    # Copy the users file
    docker cp $2 $containerID:/app/test_data/energy.json
else
    # Copy an empty array into a temp file so that the parsing script knows no file was provided
    touch empty.json
    echo "[]" > empty.json
    docker cp empty.json $containerID:/app/test_data/energy.json
    rm empty.json
fi

docker container start $containerID
mkdir -p public
docker cp $containerID:/app/index.html .
docker cp $containerID:/app/public/. public
rm images.txt
rm containers.txt
