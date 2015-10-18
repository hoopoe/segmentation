#include <set>
#include <cstdlib> //rand for mac
#include "image.h"          // image
#include "misc.h"           // rgb
#include "segment-graph.h"  // edge
#include "segmentator.h"
#include "filter.h" // smooth
//#include <math.h>

using namespace std;

// random color
rgb random_rgb()
{
    rgb c;
    c.r = (uchar)rand();
    c.g = (uchar)rand();
    c.b = (uchar)rand();
    return c;
}

// dissimilarity measure between pixels
static inline float diff(image<float> *r, image<float> *g, image<float> *b,
                         int x1, int y1, int x2, int y2) {
    return sqrt(square(imRef(r, x1, y1)-imRef(r, x2, y2)) +
                square(imRef(g, x1, y1)-imRef(g, x2, y2)) +
                square(imRef(b, x1, y1)-imRef(b, x2, y2)));
}

Segmentator::Segmentator()
{       
}

Segmentator::~Segmentator()
{
}

image<rgb>* Segmentator::segment(image<rgb> *source, float sigma, float c,
                                 int min_size, int *num_ccs)
{
    int width = source->width();
    int height = source->height();

    image<float> *r = new image<float>(width, height);
    image<float> *g = new image<float>(width, height);
    image<float> *b = new image<float>(width, height);

    // smooth each color channel
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            imRef(r, x, y) = imRef(source, x, y).r;
            imRef(g, x, y) = imRef(source, x, y).g;
            imRef(b, x, y) = imRef(source, x, y).b;
        }
    }
    image<float> *smooth_r = smooth(r, sigma);
    image<float> *smooth_g = smooth(g, sigma);
    image<float> *smooth_b = smooth(b, sigma);
    delete r;
    delete g;
    delete b;

    // build graph
    edge *edges = new edge[width*height*4];
    int num = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x < width-1) {
                edges[num].a = y * width + x;
                edges[num].b = y * width + (x+1);
                edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y);
                num++;
            }

            if (y < height-1) {
                edges[num].a = y * width + x;
                edges[num].b = (y+1) * width + x;
                edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x, y+1);
                num++;
            }

            if ((x < width-1) && (y < height-1)) {
                edges[num].a = y * width + x;
                edges[num].b = (y+1) * width + (x+1);
                edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y+1);
                num++;
            }

            if ((x < width-1) && (y > 0)) {
                edges[num].a = y * width + x;
                edges[num].b = (y-1) * width + (x+1);
                edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y-1);
                num++;
            }
        }
    }
    delete smooth_r;
    delete smooth_g;
    delete smooth_b;

    // segment
    universe *u = segment_graph(width*height, num, edges, c);

    // post process small components
    for (int i = 0; i < num; i++) {
        int a = u->find(edges[i].a);
        int b = u->find(edges[i].b);
        if ((a != b) && ((u->size(a) < min_size) || (u->size(b) < min_size)))
            u->join(a, b);
    }

    delete [] edges;
    *num_ccs = u->num_sets();

    image<rgb> *output = new image<rgb>(width, height);

    // pick random colors for each component
    rgb *colors = new rgb[width*height];
    for (int i = 0; i < width*height; i++)
      colors[i] = random_rgb();

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        int comp = u->find(y * width + x);
        imRef(output, x, y) = colors[comp];
      }
    }

    delete [] colors;
    delete u;

    return output;
}
