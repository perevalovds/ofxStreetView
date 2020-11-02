#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);

	//streetview.setLatLon(56.8251529, 60.6447997);  // Yekaterinburg
	
	//streetview.setLatLon(40.714867, -73.975910);  //

	//streetview.setLatLon(40.7577034, -73.9854863);  // Time Sq
   //streetview.setLatLon(40.75732,-73.985951);  // Time Sq
   streetview.setLatLon(40.768153,-73.981473); // Columbus Circus
   // streetview.setLatLon(40.751511,-73.993953);  // Penn Station

	//streetview.setLatLon(22.276499,114.1735439); // wanchai MTR hong kong;

   // streetview.setLatLon( 51.462088,-2.5901384 ); //stokes croft
   // streetview.setLatLon( 50.7530769,5.6964121 ); //liege netherlands border post
	 // streetview.setLatLon( 50.7531791,5.6960133 ); //liege netherlands border post  2
	//streetview.setZoom(3);

	b_drawPointCloud = true;
	b_enableLight = false;

	//fileName = "streetmesh" + ofGetTimestampString() + ".obj";
	//cout << fileName << endl;

	//obj.open(ofToDataPath(fileName), ofFile::WriteOnly);

}

//--------------------------------------------------------------
void ofApp::update() {
	streetview.update();
	mesh = streetview.getDepthMesh();
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);

	if (b_enableLight) worldLight.enable();
	cam.begin();

	ofPushMatrix();

	//I move and scale a bit - but for precise view please implement your positioning instead ofEasyCam
	float scl = 25;			//initial scale
	ofScale(scl, scl, scl);
	ofRotateX(-85);		//rotation

	if (b_drawPointCloud) {
		ofEnableDepthTest();
		
		glPointSize(4);
		streetview.draw(draw_mode);
		ofDisableDepthTest();
	}
	else {

		// db hack nov 2017
		mesh.setMode(OF_PRIMITIVE_POINTS);

		glPointSize(4);
		//glEnable(GL_POINT_SMOOTH); // use circular points instead of square points
		ofPushMatrix();
		//ofScale(-1, -1, 1);  // the projected points are 'upside down' and 'backwards'
	   // ofTranslate(0, 0, 0); // center the points a bit
		glEnable(GL_DEPTH_TEST);
		//gluPerspective(57.0, 1.5, 0.1, 20000.0); // fov,
		glShadeModel(GL_TRIANGLES);

		mesh.drawVertices();
		//mesh.drawFaces();
		//ofSetColor( 255, 255, 255);  //set render colour for unpainted points, faces and lines
		//mesh.draw();
		glDisable(GL_DEPTH_TEST);
		// mesh.clear();
		ofPopMatrix();
	}

	ofPopMatrix();
	cam.end();
	worldLight.disable();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
	//toggle point cloud view
	if (key == 'p') b_drawPointCloud = !b_drawPointCloud;

	//toggle point, wireframe, fill modes
	if (key == '1') draw_mode = OF_MESH_POINTS;
	if (key == '2') draw_mode = OF_MESH_WIREFRAME;
	if (key == '3') draw_mode = OF_MESH_FILL;
	
	//toggle fullscreen
	if (key == 'f') ofToggleFullscreen();

	//toggle light
	if (key == 'l') b_enableLight = !b_enableLight;

	//save depth and panoramic images to PNG files
	if (key == 's') {
		//save depth map to file
		string depth_file = "depth.png";
		cout << "Saving " << depth_file << endl;
		streetview.getDepthMap().save(depth_file);

		//save pano to file
		string pano_file = "pano.png";
		cout << "Saving " << pano_file << endl;
		ofSaveImage(streetview.getTexturePixels(), pano_file);

		//exportOBJ(mesh);
	}

}

//-----------------
void ofApp::exportOBJ(ofMesh &mesh) {
	/*
	//obj.open(ofToDataPath(name),ofFile::WriteOnly);
	obj << "#vertices\n";

	for (int i = 0; i < mesh.getNumVertices(); i++) {
		ofVec3f v = mesh.getVertex(i);
		obj << "v " + ofToString(v.x) + " " + ofToString(v.y) + " " + ofToString(v.z) + "\n";
	}
	obj << "#faces\n";
	for (int i = 0; i < mesh.getNumIndices(); i += 3)
		obj << "f " + ofToString(mesh.getIndex(i)) + " " + ofToString(mesh.getIndex(i + 1)) + " " + ofToString(mesh.getIndex(i + 2)) + "\n";
	obj << "\n";
	obj.close();
	cout << "wrote obj file" << endl;
	*/
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
