'''
Created on 8.9.2009

@author: jonnena
'''

import httplib

from httplib import HTTPConnection, HTTPException
from webdav import WebdavClient
from webdav.Connection import WebdavError, AuthorizationError
from PyQt4.QtGui import QInputDialog, QLineEdit

CABLEBEACH_IDENTITY_HEADER = "CableBeach-Identity"
CABLEBEACH_WEBDAV_HEADER = "CableBeach-WebDavInventory"

class HTTPClient(object):
    
    def __init__(self):
        self.httpclient = None
        self.identityQueryString = None
        pass
    
    def setupConnection(self, host, identityType, identity = None, firstName = None, lastName = None):
        strict = None
        port = None
        timeout = 10
        if (self.createRequestURL(identityType, identity, firstName, lastName) == False):
            return False
        """ host = 'www.example.com:port' NO http:// or / at end """
        self.httpclient = HTTPConnection(host, port, strict, timeout)
    
    def createRequestURL(self, identityType, identity, firstName, lastName):
        self.identityQueryString = "/services/webdav?type=" + identityType + "&"
        if (identityType == "openid" and identity != None):
            self.identityQueryString += "identity=" + identity
        elif (identityType == "normal" and firstName != None and lastName != None):
            self.identityQueryString += "firstname=" + firstName + "&lastname=" + lastName
        else:
            return False
        
    def requestIdentityAndWebDavURL(self):
        identityurl = None
        webdavurl = None
        if (self.httpclient != None):
            try:
                self.httpclient.request("GET", self.identityQueryString)
            except httplib.socket.error:
                raise httplib.error
            response = self.httpclient.getresponse()
            if (response.status == httplib.OK):
                identityurl = response.getheader(CABLEBEACH_IDENTITY_HEADER)
                webdavurl = response.getheader(CABLEBEACH_WEBDAV_HEADER)
            elif (response.status == httplib.NOT_FOUND):
                identityurl = None
                webdavurl = None
            self.httpclient.close()
            return identityurl, webdavurl

class WebDavClient(object):
        
    connection = None
    resource = None
    url = None
    user = None
    password = None
    
    def __init__(self, identityurl, webdavurl):
        self.user = identityurl
        self.url = webdavurl
        
    def setHostAndUser(self, user, url):
        self.user = user
        self.url = url
    
    def setupConnection(self):
        if (self.connection != None):
            self.connection = None
        try:
            self.resource = WebdavClient.CollectionStorer(self.url, self.connection)
            self.determineAuthentication()
            return True
        except WebdavError:
            raise HTTPException
            
    def determineAuthentication(self):
        authFailures = 0
        while authFailures < 2:
            try:
                self.resource.getCollectionContents()
            except AuthorizationError as e:                           
                if e.authType == "Basic":
                    #self.password = QInputDialog.getText(None, "WebDav Inventory Password", "WebDav Inventory is asking for Basic authentication\n\nPlease give your password:", QLineEdit.Normal, "")
                    self.password = ""
                    self.resource.connection.addBasicAuthorization(self.user, str(self.password))
                elif e.authType == "Digest":
                    #self.password = QInputDialog.getText(None, "WebDav Inventory Password", "WebDav Inventory is asking for Digest authentication\n\nPlease give your password:", QLineEdit.Normal, "")
                    self.password = ""
                    info = WebdavClient.parseDigestAuthInfo(e.authInfo)
                    self.resource.connection.addDigestAuthorization(self.user, self.password, realm=info["realm"], qop=info["qop"], nonce=info["nonce"])
                else:
                    raise WebdavError
            except HTTPException:
                raise HTTPException  
            authFailures += 1
        
    def listResources(self, path):
        if (path != None):
            if (self.setCollectionStorerToPath(path)):
                try:
                    resourceList = self.resource.listResources()
                    return resourceList
                except WebdavError:
                    return False 
            else:
                return False
        else:
            try:
                resourceList = self.resource.listResources()
                return resourceList
            except WebdavError:
                return False 

    
    def setCollectionStorerToPath(self, path):
        try:
            self.resource = WebdavClient.CollectionStorer(self.url + path, self.resource.connection)
            return True
        except WebdavError:
            return False
    
    def downloadFile(self, filePath, collectionWebDavPath, resourceWebDavName):
        if ( self.setCollectionStorerToPath(collectionWebDavPath) ):
            try:
                webdavResource = self.resource.getResourceStorer(resourceWebDavName)
                webdavResource.downloadFile(filePath + "/" + resourceWebDavName)
                return True
            except Exception:
                return False
        else:
            return False
        
    def uploadFile(self, filePath, collectionWebDavPath, resourceWebDavName):
        try:
            dataFile= open(filePath, 'rb')
        except IOError:
            return False
        if ( self.setCollectionStorerToPath(collectionWebDavPath) ):
            try:     
                self.resource.addResource(resourceWebDavName, dataFile.read())
                dataFile.close()
                return True
            except WebdavError, IOError:
                return False
        else:
            return False
        
    def createDirectory(self, collectionWebDavPath, directoryName):
        if ( self.setCollectionStorerToPath(collectionWebDavPath) ):
            try:
                self.resource.addCollection(directoryName)
                return True
            except WebdavError:
                return False
            
    def deleteResource(self, collectionWebDavPath, resourceWebDavName):
        if (self.setCollectionStorerToPath(collectionWebDavPath)):
            try:
                webdavResource = self.resource.getResourceStorer(resourceWebDavName)
                webdavResource.delete()
                return True
            except WebdavError:
                return False
        else:
            return False
        
    def deleteDirectory(self, collectionWebDavPath, collectionName):
        if (self.setCollectionStorerToPath(collectionWebDavPath)):
            try:
                self.resource.deleteResource(collectionName)
                return True
            except WebdavError:
                return False