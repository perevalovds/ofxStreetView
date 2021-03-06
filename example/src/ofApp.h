#pragma once

#include "ofMain.h"
#include "ofxStreetView.h"

class ofApp : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
		
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    ofEasyCam cam;
    
    ofxStreetView streetview;
    
    //db hack nov 2017
    ofLight worldLight;
    ofMesh mesh;
    ofFile obj;
    void exportOBJ(ofMesh &mesh);
    bool b_drawPointCloud, b_enableLight;
    string fileName;

	//OF_MESH_POINTS,
	//OF_MESH_WIREFRAME,
	//OF_MESH_FILL
	ofPolyRenderMode draw_mode = OF_MESH_FILL;
};
