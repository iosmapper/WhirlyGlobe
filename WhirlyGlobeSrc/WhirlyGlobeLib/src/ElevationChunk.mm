/*
 *  ElevationChunk.mm
 *  WhirlyGlobeLib
 *
 *  Created by Steve Gifford on 6/24/13.
 *  Copyright 2011-2013 mousebird consulting. All rights reserved.
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

#import "ElevationChunk.h"
#import "sqlhelpers.h"

using namespace Eigen;
using namespace WhirlyKit;

typedef enum {WhirlyKitElevationFloats,WhirlyKitElevationShorts} WhirlyKitElevationFormat;

// Just add some properties so the logging in interpolate works
@interface WhirlyKitElevationChunk () {
    
}

@property int tilelevel;
@property int tilecol;
@property int tilerow;

@end


@implementation WhirlyKitElevationChunk
{
    WhirlyKitElevationFormat dataType;
    NSData *data;
}

+ (WhirlyKitElevationChunk *)ElevationChunkWithRandomData
{
    int numX = 20;
    int numY = 20;
    float floatArray[numX*numY];
    for (unsigned int ii=0;ii<numX*numY;ii++)
        floatArray[ii] = drand48()*30000;
    NSMutableData *data = [[NSMutableData alloc] initWithBytes:floatArray length:sizeof(float)*numX*numY];
    WhirlyKitElevationChunk *chunk = [[WhirlyKitElevationChunk alloc] initWithFloatData:data sizeX:numX sizeY:numY];
    
    return chunk;
}

/*
 This is a giant hack to test terrain.  Would not implement this way, just trying to figure out terrain system
 */
+(WhirlyKitElevationChunk *)loadElevationChunkForLevel:(int)level col:(int)col row:(int)row {
    sqlite3 *sqlDb;
    
    
    NSString *path = [[NSBundle mainBundle] pathForResource:@"terraintest1_x" ofType:@"sqlite"];
    if (sqlite3_open([path cStringUsingEncoding:NSASCIIStringEncoding],&sqlDb) != SQLITE_OK)
    {
        return nil;
    }
    
    NSData *terrainData = nil;
    int numx = 20;
    int numy = 20;
    int ymax = pow(2.0, level);
    int y = ymax - row - 1;
    
    @try {
        
        if ((level == 11) && (col == 618) && (y == 742))  {
            // Mt. Washington
            NSLog(@"Requesting Terrain Data  zoom:%i x:%i y:%i", level, col, y);
        }
        sqlhelpers::StatementRead readStmt(sqlDb,[NSString stringWithFormat:@"SELECT terraindata from wgterrain where zoom=%i AND tilex=%i AND tiley=%i;",level,col,y]);
        if (readStmt.stepRow()) {
            //            NSLog(@"Loaded Data  zoom:%i x:%i y:%i", level, col, y);
            terrainData = readStmt.getBlob();
        }
        
        sqlhelpers::StatementRead readNumXStmt(sqlDb,[NSString stringWithFormat:@"SELECT numx from wgterrain where zoom=%i AND tilex=%i AND tiley=%i;",level,col,y]);
        if (readNumXStmt.stepRow()) {
            //            NSLog(@"Loaded Data  zoom:%i x:%i y:%i", level, col, y);
            numx = readNumXStmt.getInt();
        }
        
        sqlhelpers::StatementRead readNumYStmt(sqlDb,[NSString stringWithFormat:@"SELECT numy from wgterrain where zoom=%i AND tilex=%i AND tiley=%i;",level,col,y]);
        if (readNumYStmt.stepRow()) {
            //            NSLog(@"Loaded Data  zoom:%i x:%i y:%i", level, col, y);
            numy = readNumYStmt.getInt();
        }
        
        
        
    } @finally {
        sqlite3_close(sqlDb);
    }
    
    if (!terrainData) {
        return nil;
    }
    
    //    return [WhirlyKitElevationChunk ElevationChunkWithRandomData];
    
    if ((level == 11) && (col == 618) && (y == 742))  {
        // Mt. Washington
        for (int yy=0; yy < numy; yy++) {
            NSMutableString *currentLine = [NSMutableString stringWithFormat:@"%i:", yy];
            for (int xx=0; xx < numx; xx++) {
                float value = ((float *)[terrainData bytes])[yy*numx+xx];
                [currentLine appendFormat:@" %4.0f", value];
            }
            NSLog(@"%@", currentLine);
        }
        NSLog(@"Done");
        
    }
    
    
    
    WhirlyKitElevationChunk *chunk = [[WhirlyKitElevationChunk alloc] initWithFloatData:terrainData
                                                                                  sizeX:numx
                                                                                  sizeY:numy];
    chunk.tilecol = col;
    chunk.tilerow = y;
    chunk.tilelevel = level;
    return chunk;
    
    
}



- (id)initWithFloatData:(NSData *)inData sizeX:(int)sizeX sizeY:(int)sizeY
{
    self = [super init];
    if (!self)
        return nil;
    
    _numX = sizeX;
    _numY = sizeY;
    dataType = WhirlyKitElevationFloats;
    data = inData;
    _noDataValue = -10000000;
    
    return self;
}

- (id)initWithShortData:(NSData *)inData sizeX:(int)sizeX sizeY:(int)sizeY
{
    self = [super init];
    if (!self)
        return nil;
    
    _numX = sizeX;
    _numY = sizeY;
    dataType = WhirlyKitElevationShorts;
    data = inData;
    _noDataValue = -10000000;
    
    return self;    
}


/// Return a single elevation at the given location
- (float)elevationAtX:(int)x y:(int)y
{
    if (!data)
        return 0.0;
    if (x < 0 || y < 0 || x >= _numX || y >= _numY)
        return 0.0;
    
    float ret = 0.0;
    switch (dataType)
    {
        case WhirlyKitElevationShorts:
            ret = ((short *)[data bytes])[y*_numX+x];
            break;
        case WhirlyKitElevationFloats:
            ret = ((float *)[data bytes])[y*_numX+x];
            break;
    }
    
    if (ret == _noDataValue)
        ret = 0.0;
    
    return ret;
}

- (float)interpolateElevationAtX:(float)x y:(float)y
{
    if (!data)
        return 0.0;
    if (x < 0.0 || y < 0.0 || x > _numX || y > _numY)
        return 0.0;
    
    float elevs[4];
    int minX = (int)x;
    int minY = (int)y;
    elevs[0] = [self elevationAtX:minX y:minY];
    elevs[1] = [self elevationAtX:minX+1 y:minY];
    elevs[2] = [self elevationAtX:minX+1 y:minY+1];
    elevs[3] = [self elevationAtX:minX y:minY+1];
    
    // Interpolate a new value
    float ta = (x-minX);
    float tb = (y-minY);
    float elev0 = (elevs[1]-elevs[0])*ta + elevs[0];
    float elev1 = (elevs[2]-elevs[3])*ta + elevs[3];
    float ret = (elev1-elev0)*tb + elev0;
    
    
    if ((self.tilelevel == 11) && (self.tilecol == 618) && (self.tilerow == 742))  {
        if (y == 19.5) {
            // This is here so I can set a breakpoint
//            NSLog(@"19.5");
        }
        // This shows the various atomic requests for this particular tile
        NSLog(@"x:%f y:%f alt:%f", x, y, ret);
    }

    return ret;
}


@end
