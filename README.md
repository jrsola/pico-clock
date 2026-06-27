## Pico Container C
### Template to develop in C/C++ with a Raspberry Pi Pico and using Docker and Visual Studio Code 

Based on: [PicoDevEnv](https://github.com/AppacYazilim/PicoDevEnv)

#### Problem:
Setting up the enviroment to develop in C/C++ for the Pico can be a hassle. Rather than installing and setting up your environment each time, I wanted to have a development environment that was ready to go  is to solve the problem of having to download serveral programs and set up the different SDK to develop in C/C++ with a Raspberry Pi Pico.

#### Pre-requisites:  
* Docker installed  
* Visual Studio Code Installed
* Dev Containers Extension installed


#### How to use:

1. Change to a directory where you want to create your new project  
`mkdir <my_project> && cd <my_project>` 
 
2. Download this repository  
    `curl -L https://codeload.github.com/jrsola/pico-project/tar.gz/refs/heads/main \ `  
     ` | tar -xz --strip-components=1`
  
3. Initialize the git information  
`git init`   
`git add .`  
`git commit -m "Initial commit"`  

4. Initialize this README.MD file  
`echo "README text here" > README.md`  

5. Open the directory in Visual Studio Code.  

6. Edit the file `project_name.txt` and add the name of your pico project

7. (Optional) Create a file `wifi_credentials.txt` if you are developing for a Pico with Wifi. It just requires two lines: first one for the WiFi Network Name and the second one with the password.

8. Open in Container from Visual Studio Code. The first time it will build the docker image and take some time to download all.

9. Once it's loaded, the environment will set up and will compile tools that are missing (like picotool). Ignore the problem you'll see it shows before the tools are compiled.

10. Click on Build in the bottom bar in VSC and it should compile and build the .elf file you can then copy to the pico to test your program.
