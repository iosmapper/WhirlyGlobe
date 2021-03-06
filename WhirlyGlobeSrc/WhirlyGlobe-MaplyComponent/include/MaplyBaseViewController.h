/*
 *  MaplyBaseViewController.h
 *  MaplyComponent
 *
 *  Created by Steve Gifford on 12/14/12.
 *  Copyright 2012 mousebird consulting
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#import <UIKit/UIKit.h>
#import "MaplyCoordinate.h"
#import "MaplyScreenMarker.h"
#import "MaplyVectorObject.h"
#import "MaplyViewTracker.h"
#import "MaplyComponentObject.h"
#import "MaplySharedAttributes.h"
#import "MaplyViewControllerLayer.h"
#import "MaplyLight.h"
#import "MaplyShader.h"
#import "MaplyActiveObject.h"

/// Where we'd like an add to be executed.  If you need immediate feedback,
///  then be on the main thread and use MaplyThreadCurrent.  Any is the default. 
typedef enum {MaplyThreadCurrent,MaplyThreadAny} MaplyThreadMode;

/** The MaplyBaseViewController is the base class for the Maply and WhirlyGlobe
    view controllers.  Most of its functionality is private, but you can use
    those view controllers instead.
 */
@interface MaplyBaseViewController : UIViewController

/// Set selection support on or off here
@property(nonatomic,assign) bool selection;

/// Set the globe view's background color.
/// Black, by default.
@property (nonatomic,strong) UIColor *clearColor;

/// Set the frame interval passed to the displaylink
/// 1 == 60fps, 2 == 30fps, 3 == 20fps
@property (nonatomic,assign) int frameInterval;

/// Clear all the currently active lights.
/// There are a default set of lights, so you'll need to do this before adding your own.
- (void)clearLights;

/// Add the given light to the lighting model
- (void)addLight:(MaplyLight *)light;

/// Remove the given light (assuming it's active) from the lighting model
- (void)removeLight:(MaplyLight *)light;

/// Add rendering and other general hints for the globe view controller.
- (void)setHints:(NSDictionary *)hintsDict;

/// Add a group of screen (2D) markers with description dictionary
- (MaplyComponentObject *)addScreenMarkers:(NSArray *)markers desc:(NSDictionary *)desc;

/// Add a group of screen (2D) markers with description dictionary using the given thread mode
- (MaplyComponentObject *)addScreenMarkers:(NSArray *)markers desc:(NSDictionary *)desc mode:(MaplyThreadMode)threadMode;

/// Add a group of 3D markers 
- (MaplyComponentObject *)addMarkers:(NSArray *)markers desc:(NSDictionary *)desc;

/// Add a group of 3D markers using the given thread mode
- (MaplyComponentObject *)addMarkers:(NSArray *)markers desc:(NSDictionary *)desc mode:(MaplyThreadMode)threadMode;

/// Add a group of screen (2D) labels with description dictionary 
- (MaplyComponentObject *)addScreenLabels:(NSArray *)labels desc:(NSDictionary *)desc;

/// Add a group of screen (2D) labels with description dictionary using the given thread mode
- (MaplyComponentObject *)addScreenLabels:(NSArray *)labels desc:(NSDictionary *)desc mode:(MaplyThreadMode)threadMode;

/// Add a group of 3D labels with description dictionary 
- (MaplyComponentObject *)addLabels:(NSArray *)labels desc:(NSDictionary *)desc;

/// Add a group of 3D labels with description dictionary using the given thread mode
- (MaplyComponentObject *)addLabels:(NSArray *)labels desc:(NSDictionary *)desc mode:(MaplyThreadMode)threadMode;

/// Add one or more vectors with the given description dictionary 
- (MaplyComponentObject *)addVectors:(NSArray *)vectors desc:(NSDictionary *)desc;

/// Add one or more vectors with the given description dictionary using the given thread mode
- (MaplyComponentObject *)addVectors:(NSArray *)vectors desc:(NSDictionary *)desc mode:(MaplyThreadMode)threadMode;

/// Add one or more shapes with a description dictionary
- (MaplyComponentObject *)addShapes:(NSArray *)shapes desc:(NSDictionary *)desc;

/// Add one or more shapes with a description dictionary using the given thread mode
- (MaplyComponentObject *)addShapes:(NSArray *)shapes desc:(NSDictionary *)desc mode:(MaplyThreadMode)threadMode;

/// Add one or more stickers with a description dictionary.
- (MaplyComponentObject *)addStickers:(NSArray *)stickers desc:(NSDictionary *)desc;

/// Add one or more stickers with a description dictionary using the given thread mode
- (MaplyComponentObject *)addStickers:(NSArray *)stickers desc:(NSDictionary *)desc mode:(MaplyThreadMode)threadMode;

/// Add one or more vectors, but only for selection
- (MaplyComponentObject *)addSelectionVectors:(NSArray *)vectors;

/// Change the representation for the given vector object(s).
/// Only a few things are changeable, such as color
- (void)changeVector:(MaplyComponentObject *)compObj desc:(NSDictionary *)desc;

/// This version takes a description dictionary to use as override.  This is thread safe.
- (MaplyComponentObject *)addLoftedPolys:(NSArray *)polys key:(NSString *)key cache:(MaplyVectorDatabase *)cacheDb desc:(NSDictionary *)desc;

/// Add a view to track to a particular location
- (void)addViewTracker:(MaplyViewTracker *)viewTrack;

/// Remove the view tracker associated with the given UIView
- (void)removeViewTrackForView:(UIView *)view;

/// Set the max number of objects for the layout engine to display.
/// This will only affect objects that have an importance set
- (void)setMaxLayoutObjects:(int)maxLayoutObjects;

/// Remove a single object
- (void)removeObject:(MaplyComponentObject *)theObj;

/// Remove the given objects
- (void)removeObjects:(NSArray *)theObjs;

/// Remove an array of data objects using the given thread mode
- (void)removeObjects:(NSArray *)theObjs mode:(MaplyThreadMode)threadMode;

/// Disable a group of objects all at once
- (void)disableObjects:(NSArray *)theObjs mode:(MaplyThreadMode)threadMode;

/// Enable a group of objects all at once
- (void)enableObjects:(NSArray *)theObjs mode:(MaplyThreadMode)threadMode;

/// Add an active object.  These are used for editing and act
///  only on the main thread.
- (void)addActiveObject:(MaplyActiveObject *)theObj;

/// Remove an active object
- (void)removeActiveObject:(MaplyActiveObject *)theObj;

/// Remove an array of active objects
- (void)removeActiveObjects:(NSArray *)theObjs;

/// Add a single layer.  The layer will be added to the layer thread and its contents
///  will show up some time after that.  Depends on what it has to load and from where.
- (bool)addLayer:(MaplyViewControllerLayer *)layer;

/// Remove a single layer
- (void)removeLayer:(MaplyViewControllerLayer *)layer;

/// Remove all the base layers (e.g map layers)
- (void)removeAllLayers;

/// This utility routine will convert a lat/lon (in radians) to display coordinates
- (MaplyCoordinate3d)displayPointFromGeo:(MaplyCoordinate)geoCoord;

/// Start animation (only if it's been paused)
- (void)startAnimation;

/// Pause animation (probably because we're going into the background)
- (void)stopAnimation;

/// Add a compiled shader.  We'll refer to it by the scene name.
/// If you use one of the well known scene names, you can replace the defaults.
- (void)addShaderProgram:(MaplyShader *)shader sceneName:(NSString *)sceneName;

/// Turn on/off performance output (goes to the log periodically)
@property (nonatomic,assign) bool performanceOutput;

/// Add a quad tree paged earth layer with MapBox Tiles on top.
- (MaplyViewControllerLayer *)addQuadEarthLayerWithMBTiles:(NSString *)name __deprecated;

/// Add a quad tree paged earth layer from the remote source with the given attributes
- (MaplyViewControllerLayer *)addQuadEarthLayerWithRemoteSource:(NSString *)baseURL imageExt:(NSString *)ext cache:(NSString *)cachdDir minZoom:(int)minZoom maxZoom:(int)maxZoom __deprecated;

/// Add a quad tree paged earth layer as defined by a JSON description.  cacheDir is where we'll cache local files.
- (MaplyViewControllerLayer *)addQuadEarthLayerWithRemoteSource:(NSDictionary *)jsonDict cache:(NSString *)cacheDir __deprecated;

/// Add a quad tree paged earth layer with a local source generated by ImageChopper
- (MaplyViewControllerLayer *)addQuadSphericalEarthLayerWithImageSet:(NSString *)imageSet __deprecated;

/// Add visual defaults for the screen markers
//- (void)setScreenMarkerDesc:(NSDictionary *)desc __deprecated;

/// Add visual defaults for the markers
//- (void)setMarkerDesc:(NSDictionary *)desc __deprecated;

/// Add a group of 3D markers
//- (MaplyComponentObject *)addMarkers:(NSArray *)markers __deprecated;

/// Add visual defaults for the screen labels
//- (void)setScreenLabelDesc:(NSDictionary *)desc __deprecated;

/// Add a group of screen (2D) labels
//- (MaplyComponentObject *)addScreenLabels:(NSArray *)labels __deprecated;

/// Add visual defaults for the labels
//- (void)setLabelDesc:(NSDictionary *)desc __deprecated;

/// Add a group of 3D labels
//- (MaplyComponentObject *)addLabels:(NSArray *)labels __deprecated;

/// Add visual defaults for the vectors
//- (void)setVectorDesc:(NSDictionary *)desc __deprecated;

/// Add one or more vectors
//- (MaplyComponentObject *)addVectors:(NSArray *)vectors __deprecated;

/// Add visual defaults for the shapes
//- (void)setShapeDesc:(NSDictionary *)desc __deprecated;

/// Add one or more shapes
//- (MaplyComponentObject *)addShapes:(NSArray *)shapes __deprecated;

/// Add visual defaults for the stickers
//- (void)setStickerDesc:(NSDictionary *)desc __deprecated;

/// Add one or more stickers
//- (MaplyComponentObject *)addStickers:(NSArray *)stickers __deprecated;

/// Add visual defaults for the lofted polys
//- (void)setLoftedPolyDesc:(NSDictionary *)desc __deprecated;

/// Add one or more lofted polys.
/// If you pass in a vector database, we'll attempt to cache
///  the generated data with 'key' for speed.  The vector database should
///  be where the polys originated.  nil is acceptable for both key and cacheDb.
//- (MaplyComponentObject *)addLoftedPolys:(NSArray *)polys key:(NSString *)key cache:(MaplyVectorDatabase *)cacheDb __deprecated;

/// Add a group of screen (2D) markers
//- (MaplyComponentObject *)addScreenMarkers:(NSArray *)markers __deprecated;

@end
