# Tethys

An open source dive computer


See the dev post for the full project: https://devpost.com/software/tethys?ref_content=my-projects-tab&ref_feature=my_projects

Run main.c, which can be found in the src folder

The data visualizer is found in the dashboard folder (with its own readme for more information)

### Building the code
(On VSCode) Use platformio, and run build & upload (runs make under the hood, but this is a nice wrapper for `make build && avrdude flash && minicom -b 9600`)