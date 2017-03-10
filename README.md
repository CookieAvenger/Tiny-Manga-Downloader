# Tiny-Manga-Downloader  
  
Tiny C program to download manga seires and update already downloaded ones.  
Can save each chapter as a comic book archive, and can delete duplicate images
(works for some scanlator images).  
Experimental image duplication finder to try and remove all possible
scanlator images  
Some sort of chapter selection option to come  
  
kissmanga.com and mangasee supported (can request other sites, not too hard to impliment)  
Should run on all posix systems (untested) - only being tested on Ubuntu  
  
##To Install  
Download latest tar.gz file from releases:  
Navigate to downloaded location  
tar -xzvf tmdl\_0.1.0.tar.gz  
cd Tiny-Manga-Downloader  
./configure  
make  
sudo make install  
optional: make clean  
  
##To uninstall  
In above Tiny-Manga-Downloader folder run  
sudo make uninstall  
  
##To Run  
Once install run with  
manga-dl  
  
And check out the man page with  
man manga-dl  
  
###Dependences to run  
cfscrape - for kissmanga.com (and by extension python) "sudo -H pip install cfscrape" should do the trick   
findimagedupes - for experimental feature "sudo apt install findimagedupes" works on Ubuntu, other distributions should have it with their respective package managers  
shasum (comes with perl)  
zip  
tar  
find  
bash (rm and mkdir too)  
  
###Dependences to compile  
libcurl  
make  
a C compiler  

##License 
  
    Copyright 2016 Krishna Shukla                                           
                                                                        
    Licensed under the Apache License, Version 2.0 (the "License");         
    you may not use this file except in compliance with the License.        
    You may obtain a copy of the License at                                 
                                                                        
    http://www.apache.org/licenses/LICENSE-2.0                          
                                                                        
    Unless required by applicable law or agreed to in writing, software     
    distributed under the License is distributed on an "AS IS" BASIS,       
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
    See the License for the specific language governing permissions and     
    limitations under the License.                                          
  
##Disclaimer
  
The developer of this application does not have any affiliation with the content providers available.  
