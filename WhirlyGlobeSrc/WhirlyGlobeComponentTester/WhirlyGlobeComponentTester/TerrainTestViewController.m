//
//  TerrainTestViewController.m
//  WhirlyGlobeComponentTester
//
//  Copyright (c) 2013 mousebird consulting. All rights reserved.
//

#import "TerrainTestViewController.h"
#import "AFJSONRequestOperation.h"


@interface TerrainTestViewController () {
    
}

@property WhirlyGlobeViewController *globeView;

@end

@implementation TerrainTestViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    self.globeView = [[WhirlyGlobeViewController alloc] init];
    self.globeView.delegate = self;
    
    [self.view addSubview:self.globeView.view];
    self.globeView.view.frame = self.view.bounds;
    //    self.globeView.view.frame = CGRectMake(0,100,768,236);
    [self addChildViewController:self.globeView];
    
    // Set the background color for the globe
    self.globeView.clearColor = [UIColor blackColor];
    
    // Start up over San Francisco
    //    [self.globeView animateToPosition:MaplyCoordinateMakeWithDegrees(-122.4192, 37.7793) time:1.0];
    
    [self.globeView setMaxLayoutObjects:10000];
    
    
    [self setupBaseLayer];
    
    
    
    
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


-(void)setupBaseLayer {
    /*
     MaplyQuadEarthWithMBTiles *layer = [[MaplyQuadEarthWithMBTiles alloc] initWithMbTiles:@"geography-class"];
     layer.handleEdges = true;
     layer.coverPoles = true;
     [self.globeView addLayer:layer];
     layer.drawPriority = 0;
     return;
     */
    
    NSString *cacheDir = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES)  objectAtIndex:0];
    NSString *jsonTileSpec = @"http://a.tiles.mapbox.com/v3/examples.map-zyt2v9k2.json";
    NSString *thisCacheDir = [NSString stringWithFormat:@"%@/mbtilessat1/",cacheDir];
    // jsonTileSpec = nil;
    
    // Fill out the cache dir if there is one
    if (thisCacheDir)
    {
        NSError *error = nil;
        [[NSFileManager defaultManager] createDirectoryAtPath:thisCacheDir withIntermediateDirectories:YES attributes:nil error:&error];
    }
    
    // If we're fetching one of the JSON tile specs, kick that off
    if (jsonTileSpec)
    {
        NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:jsonTileSpec]];
        
        AFJSONRequestOperation *operation =
        [AFJSONRequestOperation JSONRequestOperationWithRequest:request
                                                        success:^(NSURLRequest *request, NSHTTPURLResponse *response, id JSON)
         {
             // Add a quad earth paging layer based on the tile spec we just fetched
             MaplyQuadEarthWithRemoteTiles *layer = [[MaplyQuadEarthWithRemoteTiles alloc] initWithTilespec:JSON];
             layer.handleEdges = true;
             layer.cacheDir = thisCacheDir;
             [self.globeView addLayer:layer];
             layer.drawPriority = 150;
             
             self.globeView.height = 0.001;
             [self.globeView setTiltMinHeight:0.001 maxHeight:0.04 minTilt:1.21771169 maxTilt:0.0];
             [self.globeView setTilt:M_PI/4];
             [self.globeView animateToPosition:MaplyCoordinateMakeWithDegrees(-71.3032, 44.2705) time:1.0];

         }
                                                        failure:^(NSURLRequest *request, NSHTTPURLResponse *response, NSError *error, id JSON)
         {
             NSLog(@"Failed to reach JSON tile spec at: %@",jsonTileSpec);
         }
         ];
        
        [operation start];
    } else {
        MaplyQuadTestLayer *layer = [[MaplyQuadTestLayer alloc] initWithMaxZoom:17];
        [self.globeView addLayer:layer];
        layer.drawPriority = 100;

        [self.globeView animateToPosition:MaplyCoordinateMakeWithDegrees(-71.3032, 44.2705) time:1.0];
        self.globeView.height = 0.003;
        [self.globeView setTiltMinHeight:0.001 maxHeight:0.04 minTilt:1.21771169 maxTilt:0.0];
        [self.globeView setTilt:M_PI/4];

    }
    
    
    
    
    
        
    
    
    
    
}

@end
