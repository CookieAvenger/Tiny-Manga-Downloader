##!/usr/bin/env python
#import sys
import cfscrape

#scraper = cfscrape.create_scraper() # returns a requests.Session object
#fd = open("cookie", "w")
#c = cfscrape.get_cookie_string(sys.argv[1])
#fd.write(str(c))
#fd.close()  
#print(c)
print(cfscrape.get_cookie_string("http://kissmanga.com"))
