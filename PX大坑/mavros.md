sudo apt-get install ros-noetic-mavros ros-noetic-mavros-extras

wget https://raw.githubusercontent.com/mavlink/mavros/master/mavros/scripts/install_geographiclib_datasets.sh

./install_geographiclib_datasets.sh

roslaunch mavros px4.launch

roslaunch mavros apm.launch