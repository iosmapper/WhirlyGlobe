/*
 *  BillboardManager.mm
 *  WhirlyGlobeLib
 *
 *  Created by Steve Gifford on 7/30/13.
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

#import "BillboardManager.h"
#import "NSDictionary+Stuff.h"
#import "UIColor+Stuff.h"

using namespace Eigen;
using namespace WhirlyKit;

@implementation WhirlyKitBillboardInfo

- (id)initWithBillboards:(NSArray *)billboards desc:(NSDictionary *)desc
{
    self = [super init];
    if (!self)
        return nil;
    
    _billboards = billboards;
    [self parseDesc:desc];
    
    _billboardId = Identifiable::genId();
    
    return self;
}

- (void)parseDesc:(NSDictionary *)desc
{
    _color = [desc objectForKey:@"color" checkType:[UIColor class] default:[UIColor whiteColor]];
    _minVis = [desc floatForKey:@"minVis" default:DrawVisibleInvalid];
    _maxVis = [desc floatForKey:@"maxVis" default:DrawVisibleInvalid];
    _fade = [desc floatForKey:@"fade" default:0.0];
    _drawPriority = [desc intForKey:@"drawPriority" default:0];
}

@end

@implementation WhirlyKitBillboard
@end

namespace WhirlyKit
{
    
BillboardSceneRep::BillboardSceneRep()
{
}

BillboardSceneRep::BillboardSceneRep(SimpleIdentity inId)
: Identifiable(inId)
{
}

BillboardSceneRep::~BillboardSceneRep()
{
}

void BillboardSceneRep::clearContents(ChangeSet &changes)
{
    for (SimpleIDSet::iterator it = drawIDs.begin();
         it != drawIDs.end(); ++it)
        changes.push_back(new RemDrawableReq(*it));
}

BillboardDrawableBuilder::BillboardDrawableBuilder(Scene *scene,ChangeSet &changes,BillboardSceneRep *sceneRep,WhirlyKitBillboardInfo *billInfo,SimpleIdentity billboardProgram,SimpleIdentity texId)
: scene(scene), changes(changes), sceneRep(sceneRep), billInfo(billInfo), drawable(NULL), billboardProgram(billboardProgram), texId(texId)
{
}

BillboardDrawableBuilder::~BillboardDrawableBuilder()
{
    flush();
}

void BillboardDrawableBuilder::addBillboard(Point3f center,float width,float height,UIColor *inColor)
{
    CoordSystemDisplayAdapter *coordAdapter = scene->getCoordAdapter();
    
    // Get the drawable ready
    if (!drawable || (drawable->getNumPoints()+4 > MaxDrawablePoints) ||
        (drawable->getNumTris()+2 > MaxDrawableTriangles))
    {
        if (drawable)
            flush();
        
        drawable = new BillboardDrawable();
        //        drawMbr.reset();
        drawable->setType(GL_TRIANGLES);
        drawable->setVisibleRange(billInfo.minVis,billInfo.maxVis);
        drawable->setProgram(billboardProgram);
        drawable->setTexId(0,texId);
        drawable->setDrawPriority(billInfo.drawPriority);
        //        drawable->setForceZBufferOn(true);
    }
    
    RGBAColor color = [(inColor ? inColor : billInfo.color) asRGBAColor];
    
    // Normal is straight up
    Point3f localPt = coordAdapter->displayToLocal(center);
    Point3f axisY = coordAdapter->normalForLocal(localPt);
    
    float width2 = width/2.0;
    Point3f pts[4];
    TexCoord texCoords[4];
    pts[0] = Point3f(-width2,0,0);
    texCoords[0] = TexCoord(0,0);
    pts[1] = Point3f(width2,0,0);
    texCoords[1] = TexCoord(1,0);
    pts[2] = Point3f(width2,height,0);
    texCoords[2] = TexCoord(1,1);
    pts[3] = Point3f(-width2,height,0);
    texCoords[3] = TexCoord(0,1);
    
    int startPoint = drawable->getNumPoints();
    for (unsigned int ii=0;ii<4;ii++)
    {
        drawable->addPoint(center);
        drawable->addOffset(pts[ii]);
        drawable->addTexCoord(0,texCoords[ii]);
        drawable->addNormal(axisY);
        drawable->addColor(color);
    }
    drawable->addTriangle(BasicDrawable::Triangle(startPoint+0,startPoint+1,startPoint+2));
    drawable->addTriangle(BasicDrawable::Triangle(startPoint+0,startPoint+2,startPoint+3));
}

void BillboardDrawableBuilder::flush()
{
    if (drawable)
    {
        if (drawable->getNumPoints() > 0)
        {
            //            drawable->setLocalMbr(drawMbr);
            sceneRep->drawIDs.insert(drawable->getId());
            
            if (billInfo.fade > 0.0)
            {
                NSTimeInterval curTime = CFAbsoluteTimeGetCurrent();
                drawable->setFade(curTime, curTime+billInfo.fade);
            }
            changes.push_back(new AddDrawableReq(drawable));
        } else
            delete drawable;
        drawable = NULL;
    }
}


BillboardManager::BillboardManager()
{
    pthread_mutex_init(&billLock, NULL);
}

BillboardManager::~BillboardManager()
{
    pthread_mutex_destroy(&billLock);
    for (BillboardSceneRepSet::iterator it = sceneReps.begin();
         it != sceneReps.end(); ++it)
        delete *it;
    sceneReps.clear();
}

typedef std::map<SimpleIdentity,BillboardDrawableBuilder *> BuilderMap;

/// Add billboards for display
SimpleIdentity BillboardManager::addBillboards(NSArray *billboards,NSDictionary *desc,SimpleIdentity billShader,ChangeSet &changes)
{
    WhirlyKitBillboardInfo *billboardInfo = [[WhirlyKitBillboardInfo alloc] initWithBillboards:billboards desc:desc];

    BillboardSceneRep *sceneRep = new BillboardSceneRep(billboardInfo.billboardId);
    sceneRep->fade = billboardInfo.fade;
        
    // One builder per texture
    BuilderMap drawBuilders;
    
    // Work through the billboards, constructing as we go
    for (WhirlyKitBillboard *billboard in billboardInfo.billboards)
    {
        BuilderMap::iterator it = drawBuilders.find(billboard.texId);
        BillboardDrawableBuilder *drawBuilder = NULL;
        // Need a new one
        if (it == drawBuilders.end())
        {
            drawBuilder = new BillboardDrawableBuilder(scene,changes,sceneRep,billboardInfo,billShader,billboard.texId);
            drawBuilders[billboard.texId] = drawBuilder;
        } else
            drawBuilder = it->second;
        
        drawBuilder->addBillboard(billboard.center, billboard.width, billboard.height, billboard.color);
    }
    
    // Flush out the changes and tear down the builders
    for (BuilderMap::iterator it = drawBuilders.begin();
         it != drawBuilders.end(); ++it)
    {
        BillboardDrawableBuilder *drawBuilder = it->second;
        drawBuilder->flush();
        delete drawBuilder;
    }
    drawBuilders.clear();

    SimpleIdentity billID = sceneRep->getId();

    pthread_mutex_lock(&billLock);
    sceneReps.insert(sceneRep);
    pthread_mutex_unlock(&billLock);
    
    return billID;
}

/// Remove a group of billboards named by the given ID
void BillboardManager::removeBillboards(SimpleIDSet &billIDs,ChangeSet &changes)
{
    pthread_mutex_lock(&billLock);
    
    for (SimpleIDSet::iterator bit = billIDs.begin();bit != billIDs.end();++bit)
    {
        BillboardSceneRep dummyRep(*bit);
        BillboardSceneRepSet::iterator it = sceneReps.find(&dummyRep);
        NSTimeInterval curTime = CFAbsoluteTimeGetCurrent();
        if (it != sceneReps.end())
        {
            BillboardSceneRep *sceneRep = *it;
            
            if (sceneRep->fade > 0.0)
            {
                for (SimpleIDSet::iterator it = sceneRep->drawIDs.begin();
                     it != sceneRep->drawIDs.end(); ++it)
                    changes.push_back(new FadeChangeRequest(*it, curTime, curTime+sceneRep->fade));

                // Spawn off the deletion for later
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, sceneRep->fade * NSEC_PER_SEC),
                               scene->getDispatchQueue(),
                               ^{
                                   SimpleIDSet theIDs;
                                   theIDs.insert(sceneRep->getId());
                                   ChangeSet delChanges;
                                   removeBillboards(theIDs, delChanges);
                                   scene->addChangeRequests(delChanges);
                               }
                               );

                sceneRep->fade = 0.0;
            } else {
                sceneRep->clearContents(changes);
                sceneReps.erase(it);
                delete sceneRep;
            }
        }
    }
    
    pthread_mutex_unlock(&billLock);
}

}
