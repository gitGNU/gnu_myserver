import local
import error
import xml.dom.minidom
from remote import Repository
import console
import urllib

class RepositoryGenericUrl(Repository):
    def __init__ (self,url):
        Repository.__init__(self,url)
        self.__pluginsXml = None

    def __getPluginsList(self):
        plugins = urllib.urlopen(self.url + "/list.xml")
        self.__pluginsXml = xml.dom.minidom.parse(plugins)
        
    
    def synchronizeListWithRepository(self, list):
        console.write("update: "+ list.repository + '\n')
        console.write("loading ")
        localRevision = int(list.getRevision())
        if self.__pluginsXml == None:
            self.__getPluginsList()
        pluginsElement = self.__pluginsXml.getElementsByTagName("PLUGINS")
        remoteRevision = int(pluginsElement[0].getAttribute("revision"))
        console.write(". ")
        if localRevision == remoteRevision:
            console.write("local list already updated.\n")
        if localRevision > remoteRevision:
            raise error.FatalError("Local plugins list corrupted!!")
        
        if localRevision < remoteRevision:
            list.resetToEmptyListFile(`remoteRevision`)
            console.write(". ")
            
            pluginElements = self.__pluginsXml.getElementsByTagName("PLUGIN")
            plugins = [plugin.firstChild.data for plugin in pluginElements]

            console.write(". ")
            for plugin in plugins:
                url = self.url + "/src/" + plugin + "/plugin.xml"
                pluginXml =  xml.dom.minidom.parse(urllib.urlopen(url))
                console.write(". ")
                element = pluginXml.getElementsByTagName("PLUGIN")
                pluginInfo = list.addPluginWithXml(element[0])
            list.synchronizeListWithFileSystem()
            console.write("DONE.\n")

