/*
 *  TileQuadLoader.h
 *  WhirlyGlobeLib
 *
 *  Created by Steve Gifford on 4/27/12.
 *  Copyright 2011-2013 mousebird consulting
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

#import <Foundation/Foundation.h>
#import <math.h>
#import "WhirlyVector.h"
#import "TextureGroup.h"
#import "Scene.h"
#import "DataLayer.h"
#import "LayerThread.h"
#import "GlobeMath.h"
#import "sqlhelpers.h"
#import "Quadtree.h"
#import "SceneRendererES.h"
#import "QuadDisplayLayer.h"
#import "TextureAtlas.h"
#import "ElevationChunk.h"

/// @cond
@class WhirlyKitQuadTileLoader;
/// @endcond

/** Type of the image being passed to the tile loader.
    UIImage - A UIImage object.
    NSDataAsImage - An NSData object containing PNG or JPEG data.    
    WKLoadedImageNSDataRawData - An NSData object containing raw RGBA values.
    PVRTC4 - Compressed PVRTC, 4 bit, no alpha
    Placeholder - This is an empty image (so no visual representation)
                that is nonetheless "valid" so its children will be paged.
  */
typedef enum {WKLoadedImageUIImage,WKLoadedImageNSDataAsImage,WKLoadedImageNSDataRawData,WKLoadedImagePVRTC4,WKLoadedImagePlaceholder,WKLoadedImageMax} WhirlyKitLoadedImageType;

/** The Loaded Image is handed back to the Tile Loader when an image
 is finished.  It can either be loaded or empty, or something of that sort.
 */
@interface WhirlyKitLoadedImage : NSObject

/// The data we're passing back
@property (nonatomic,assign) WhirlyKitLoadedImageType type;
/// Set if there are any border pixels in the image
@property (nonatomic,assign) int borderSize;
/// The UIImage or NSData object
@property (nonatomic) NSObject *imageData;
/// Some formats contain no size info (e.g. PVRTC).  In which case, this is set
@property (nonatomic,assign) int width,height;

/// Return a loaded image made of a standard UIImage
+ (WhirlyKitLoadedImage *)LoadedImageWithUIImage:(UIImage *)image;

/// Return a loaded image made from an NSData object containing PVRTC
+ (WhirlyKitLoadedImage *)LoadedImageWithPVRTC:(NSData *)imageData size:(int)squareSize;

/// Return a loaded image that's just an empty placeholder.
/// This means there's nothing to display, but the children are valid
+ (WhirlyKitLoadedImage *)PlaceholderImage;

/// Return a loaded image made from an NSData object that contains a PNG or JPG.
/// Basically somethign that UIImage will recognize if you initialize it with one.
+ (WhirlyKitLoadedImage *)LoadedImageWithNSDataAsPNGorJPG:(NSData *)imageData;

/// Generate an appropriate texture.
/// You could overload this, just be sure to respect the border pixels.
- (WhirlyKit::Texture *)buildTexture:(int)borderSize destWidth:(int)width destHeight:(int)height;

@end

/** This is a more generic version of the Loaded Image.  It can be a single
    loaded image, a stack of them (for animation) and/or a terrain chunk.
    If you're doing a stack of images, make sure you set up the tile quad loader
    that way.
  */
@interface WhirlyKitLoadedTile : NSObject

@property (nonatomic,readonly) NSMutableArray *images;
@property (nonatomic) WhirlyKitElevationChunk *elevChunk;

@end

namespace WhirlyKit
{
    
/** The Loaded Tile is used to track tiles that have been
 loaded in to memory, but may be in various states.  It's also
 used to fill in child outlines that may be missing.
 */
class LoadedTile
{
public:
    LoadedTile();
    LoadedTile(const WhirlyKit::Quadtree::Identifier &);
    ~LoadedTile() { }
    
    /// Build the data needed for a scene representation
    void addToScene(WhirlyKitQuadTileLoader *loader,WhirlyKitQuadDisplayLayer *layer,WhirlyKit::Scene *scene,std::vector<WhirlyKitLoadedImage *>loadImages,unsigned int currentImage,WhirlyKitElevationChunk *loadElev,std::vector<WhirlyKit::ChangeRequest *> &changeRequests);
    
    /// Remove data from scene.  This just sets up the changes requests.
    /// They must still be passed to the scene
    void clearContents(WhirlyKitQuadTileLoader *loader,WhirlyKitQuadDisplayLayer *layer,WhirlyKit::Scene *scene,std::vector<WhirlyKit::ChangeRequest *> &changeRequests);
    
    /// Update what we're displaying based on the quad tree, particulary for children
    void updateContents(WhirlyKitQuadTileLoader *loader,WhirlyKitQuadDisplayLayer *layer,WhirlyKit::Quadtree *tree,std::vector<WhirlyKit::ChangeRequest *> &changeRequests);
    
    /// Switch to
    void setCurrentImage(WhirlyKitQuadTileLoader *loader,WhirlyKitQuadDisplayLayer *layer,unsigned int whichImage,std::vector<WhirlyKit::ChangeRequest *> &changeRequests);
    
    /// Dump out to the log
    void Print(WhirlyKit::Quadtree *tree);
    
    // Details of which node we're representing
    WhirlyKit::Quadtree::NodeInfo nodeInfo;
    
    /// Set if this is just a placeholder (no geometry)
    bool placeholder;    
    /// Set if this tile is in the process of loading
    bool isLoading;
    // DrawID for this parent tile
    WhirlyKit::SimpleIdentity drawId;
    // Optional ID for the skirts
    WhirlyKit::SimpleIdentity skirtDrawId;
    // Texture IDs for the parent tile
    std::vector<WhirlyKit::SimpleIdentity> texIds;
    /// If set, these are subsets of a larger dynamic texture
    std::vector<WhirlyKit::SubTexture> subTexs;
    /// If here, the elevation data needed to build geometry
    WhirlyKitElevationChunk *elevData;
    
    // IDs for the various fake child geometry
    WhirlyKit::SimpleIdentity childDrawIds[4];
    WhirlyKit::SimpleIdentity childSkirtDrawIds[4];
};

/// This is a comparison operator for sorting loaded tile pointers by
/// Quadtree node identifier.
typedef struct
{
    /// Comparison operator based on node identifier
    bool operator() (const LoadedTile *a,const LoadedTile *b)
    {
        return a->nodeInfo.ident < b->nodeInfo.ident;
    }
} LoadedTileSorter;

/// A set that sorts loaded MB Tiles by Quad tree identifier
typedef std::set<LoadedTile *,LoadedTileSorter> LoadedTileSet;

}

/** Quad Tile Image Data Source is used to load individual images
    to put on top of the simple geometry created by the quad tile loader.
 */
@protocol WhirlyKitQuadTileImageDataSource<NSObject>
/// Number of simultaneous fetches this data source can support.
/// You can change this on the fly, but it won't cancel outstanding fetches.
- (int)maxSimultaneousFetches;

@optional
/// The quad loader is letting us know to start loading the image.
/// We'll call the loader back with the image when it's ready.
/// This is now deprecated.  Used the other version.
- (void)quadTileLoader:(WhirlyKitQuadTileLoader *)quadLoader startFetchForLevel:(int)level col:(int)col row:(int)row __deprecated;

/// This version of the load method passes in a mutable dictionary.
/// Store your expensive to generate key/value pairs here.
- (void)quadTileLoader:(WhirlyKitQuadTileLoader *)quadLoader startFetchForLevel:(int)level col:(int)col row:(int)row attrs:(NSMutableDictionary *)attrs;

@end

/// Used to specify the image type for the textures we create
typedef enum {WKTileIntRGBA,WKTileUShort565,WKTileUShort4444,WKTileUShort5551,WKTileUByte,WKTilePVRTC4} WhirlyKitTileImageType;

/// How we'll scale the tiles up or down to the nearest power of 2 (square) or not at all
typedef enum {WKTileScaleUp,WKTileScaleDown,WKTileScaleFixed,WKTileScaleNone} WhirlyKitTileScaleType;

/** The Globe Quad Tile Loader responds to the Quad Loader protocol and
    creates simple terrain (chunks of the sphere) and asks for images
    to put on top.
 */
@interface WhirlyKitQuadTileLoader : NSObject<WhirlyKitQuadLoader>

/// Offset for the data being generated
@property (nonatomic,assign) int drawOffset;
/// Priority order to use in the renderer
@property (nonatomic,assign) int drawPriority;
/// If set, the point at which tile geometry will appear when zoomed in
@property (nonatomic,assign) float minVis;
/// If set, the point at which tile geometry will disappear when zoomed outfloat maxVis;
@property (nonatomic,assign) float maxVis;
/// If set, the point at which we'll stop doing updates (separate from minVis)
@property (nonatomic,assign) float minPageVis;
/// If set, the point at which we'll stop doing updates (separate from maxVis)
@property (nonatomic,assign) float maxPageVis;
/// If set, the program to use for rendering
@property (nonatomic,assign) WhirlyKit::SimpleIdentity programId;
/// If set, we'll include elevation (Z) in the drawables for shaders to use
@property (nonatomic,assign) bool includeElev;
/// If set (by default) we'll use the elevation (if provided) as real Z values on the vertices
@property (nonatomic,assign) bool useElevAsZ;
/// The number of image layers we're expecting to be given.  By default, 1
@property (nonatomic,assign) unsigned int numImages;
/// Base color for the drawables created by the layer
@property (nonatomic,assign) WhirlyKit::RGBAColor color;
/// Set this if the tile images are partially transparent
@property (nonatomic,assign) bool hasAlpha;
/// Data layer we're attached to
@property (nonatomic,weak) WhirlyKitQuadDisplayLayer *quadLayer;
/// If set, we'll ignore edge matching.
/// This can work if you're zoomed in close
@property (nonatomic,assign) bool ignoreEdgeMatching;
/// If set, we'll fill in the poles for a projection that doesn't go all the way up or down
@property (nonatomic,assign) bool coverPoles;
/// The data type of GL textures we'll be creating.  RGBA by default.
@property (nonatomic,assign) WhirlyKitTileImageType imageType;
/// If set (before we start) we'll use dynamic texture and drawable atlases
@property (nonatomic,assign) bool useDynamicAtlas;
/// If set we'll scale the input images to the nearest square power of two
@property (nonatomic,assign) WhirlyKitTileScaleType tileScale;
/// If the tile scale is fixed, this is the size it's fixed to (256 by default)
@property (nonatomic,assign) int fixedTileSize;
/// If set, the default texture atlas size.  Must be a power of two.
@property (nonatomic,assign) int textureAtlasSize;

/// Set this up with an object that'll return an image per tile
- (id)initWithDataSource:(NSObject<WhirlyKitQuadTileImageDataSource> *)imageSource;

/// Set this up with an object that'll return an image per tile and a name (for debugging)
- (id)initWithName:(NSString *)name dataSource:(NSObject<WhirlyKitQuadTileImageDataSource> *)imageSource;

/// Called when the layer shuts down
- (void)shutdownLayer:(WhirlyKitQuadDisplayLayer *)layer scene:(WhirlyKit::Scene *)scene;

/// When a data source has finished its fetch for a given image, it calls
///  this method to hand that back to the quad tile loader
/// If this isn't called in the layer thread, it will switch over to that thread first.
- (void)dataSource:(NSObject<WhirlyKitQuadTileImageDataSource> *)dataSource loadedImage:(NSData *)image pvrtcSize:(int)pvrtcSize forLevel:(int)level col:(int)col row:(int)row __deprecated;

/// When a data source has finished its fetch for a given tile, it
///  calls this method to hand the data (along with key info) back to the
///  quad tile loader.
/// You can pass back a WhirlyKitLoadedTile or a WhirlyKitLoadedImage or
///  just a WhirlyKitElevationChunk.
- (void)dataSource:(NSObject<WhirlyKitQuadTileImageDataSource> *)dataSource loadedImage:(id)loadImage forLevel:(int)level col:(int)col row:(int)row;

/// Set up the change requests to make the given image layer the active one
/// The call is thread safe
- (void)setCurrentImage:(unsigned int)newImage changes:(WhirlyKit::ChangeSet &)changeRequests;

@end
