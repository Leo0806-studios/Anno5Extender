anno2070 Mod Loader and address library


this project includes a mod loader for Anno2070 and example plugins

it allows aribrrary dlls to be loaded into anno2070 and modify game functions

build using visual studio on windows


install instructions
1. backup your files
2. rename upc_r2_loader64.dll to upc_r2_loader64_anno_original.dll
3. copy the build upc_r2_loader64.dll in your anno root (or take it from the release archive once i can be bothered to set it up)
4. copy the Anno5Extender.dll into the anno root folder
5. steps past here might be unnecesary in the future
6. create a folder called Anno5Extender in the root anno dir



install plugins 
1. plugins have to be one level deep inside the plugin folder so /AnnoExtender/Plugins/ModName/ModName.dll
2. creat the neccecary folder and copy the file
3. launch the game





ai disclosure
ai was used for assistance in reverse engineering, writing the dll proxy and generate_msvc_bindings.py
all other code inside the project is handwritten and all/most ai written code is reviewed by me