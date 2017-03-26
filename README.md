# Tiny-Manga-Downloader   
  
Tiny C program to download manga seires and update already downloaded ones.  
Can save each chapter as a comic book archive, and can delete duplicate images
(works for some scanlator images).  
Experimental image duplication finder to try and remove all possible
scanlator images  
Automatic program updation included  
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
   
##To Update to latest stable  
sudo manga-dl update  
   
##To uninstall  
In above Tiny-Manga-Downloader folder run  
sudo make uninstall  
  
##To Run  
Once install run with  
manga-dl  
  
And check out the man page with  
man manga-dl  
  
###Dependences to run  
cfscrape - for kissmanga.com (and by extension python 2.7) "sudo -H pip install cfscrape" should do the trick   
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
    
##What's planned to come  
  
0.1.3  
   
    - Don't update manga marked finished online  
    - Add to http request "if not changed scince (date)" and store that date and weather everything was downloaded on settings file, this way if every chapter has been downloaded and page has not changed, don't bother :P  
    - allow multiple download threads, allow user to specify - try and make a method to work out optimal   
    - Thread and interleive kissmanga chapter decryption, so decrypt first link, start download, while downloading first, decrypt next link and so on
    - add option for downloading backwards, to stop after n chapters, to stop at chapter called s  
  
0.1.4 
   
    - Clean up code (and restructure)!! -- really need to do, changed design and features too often while making  
    - Start using more regex  
  
0.2.0  
  
    - add an option to be able to select chapters, use a terminal graphics library for this  

##Things to fix (that I dunno how to)  
  
    - experimental feature that uses findimagedupes - in one experiment, running -e again deleted everything already checked even though it's already on database, one possible reason may be metadata checks - the time stamps change as I unzip it and rezip it (on rezip I set it to lose all metadata) - havn't tried disabling and renabling it yet, but if that is the case may not be a full fix man pages for zip says not all extended file attirbutes are copied any - any thing it doesn't understand zip strips anyway, and it adds system specific file attributes when unzipping  
    - End of main method I have to fork and exec in order to keep terminal in attention, if I just exec it I lose the terminal focus, wnat to keep focus and just exec :/  
    - to run autoinstall through app gives a bad line descriptor error in configure script :/
        its line 560 - "test -n "$DJDIR" || exec 7<&0 </dev/null", changing it to 0<&0 instead of 7<&0 fixes it...
        but dunno why... hah ah... =S
  
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
