#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetVerticalSync(true);
    ofEnableDepthTest();
    
    //streetview.setLatLon(40.75732,-73.985951);  // Time Sq
   //streetview.setLatLon(40.768153,-73.981473); // Columbus Circus
   // streetview.setLatLon(40.751511,-73.993953);  // Penn Station
    
    //streetview.setLatLon(22.276499,114.1735439); // wanchai MTR hong kong;
    
    streetview.setLatLon( 51.462088,-2.5901384 ); //stokes croft
    streetview.setZoom(3);
    
    b_drawPointCloud, b_enableLight = false;
    
     fileName = "streetmesh" + ofGetTimestampString() + ".obj";
    cout << fileName << endl;

    obj.open(ofToDataPath(fileName),ofFile::WriteOnly);
    
}

//--------------------------------------------------------------
void ofApp::update(){
    streetview.update();
    
    //streetview.setUseTexture(false);
    
    mesh = streetview.getDethMesh();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    
    if (b_enableLight) worldLight.enable();
    cam.begin();
    
    if (b_drawPointCloud) {
    streetview.draw();
    } else {
    
    // db hack nov 2017
    mesh.setMode(OF_PRIMITIVE_POINTS);

    glPointSize(4);
    //glEnable(GL_POINT_SMOOTH); // use circular points instead of square points
    ofPushMatrix();
   // ofScale(1, -1, -1);  // the projected points are 'upside down' and 'backwards'
   // ofTranslate(0, 0, 0); // center the points a bit
    glEnable(GL_DEPTH_TEST);
    //gluPerspective(57.0, 1.5, 0.1, 20000.0); // fov,
    glShadeModel(GL_TRIANGLES);
    
    //mesh.drawVertices();
    mesh.drawFaces();
    ofSetColor( 255, 255, 255);  //set render colour for unpainted points, faces and lines
    mesh.draw();
    glDisable(GL_DEPTH_TEST);
   // mesh.clear();
    ofPopMatrix();
    }
    cam.end();
    worldLight.disable();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    switch (key) {
            
        case 'p':
        case 'P':
            b_drawPointCloud =!b_drawPointCloud;
            break;
            
            
        case 'f':
        case 'F':
            ofToggleFullscreen();
            break;
            
        case 'l':
        case 'L':
            b_enableLight = !b_enableLight;
            break;
            
            case 's':
            case 'S':
            
        exportOBJ(mesh);
    }
    
}

//-----------------
void ofApp::exportOBJ(ofMesh &mesh){

    //obj.open(ofToDataPath(name),ofFile::WriteOnly);
    obj << "#vertices\n";
    
    for(int i = 0 ; i < mesh.getNumVertices(); i++) {
        ofVec3f v = mesh.getVertex(i);
        obj << "v " + ofToString(v.x) + " " + ofToString(v.y) + " " + ofToString(v.z) + "\n";
    }
    obj << "#faces\n";
    for(int i = 0 ; i < mesh.getNumIndices(); i += 3)
        obj << "f " + ofToString(mesh.getIndex(i)) + " " + ofToString(mesh.getIndex(i+1)) + " " + ofToString(mesh.getIndex(i+2)) + "\n";
    obj << "\n";
    obj.close();
    cout << "wrote obj file"  << endl;
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
