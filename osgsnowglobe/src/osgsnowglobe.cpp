#include <osg/Notify>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarth/Registry>

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc,argv);

    osg::Node* earthNode = osgDB::readNodeFile("snowglobe.earth");
    if (!earthNode)
        return 1;

    osg::Group* root = new osg::Group();
    root->addChild(earthNode);
    
    osgViewer::Viewer viewer(arguments);
    viewer.setCameraManipulator(new osgEarth::Util::EarthManipulator());
    viewer.setSceneData(root);

    // add some stock OSG handlers
    viewer.addEventHandler(new osgViewer::StatsHandler());
    viewer.addEventHandler(new osgViewer::WindowSizeHandler());
    viewer.addEventHandler(new osgViewer::ThreadingHandler());
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

    return viewer.run();
}
