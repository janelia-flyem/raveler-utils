"""
Utility function relabel_superpixels_to_bodies(), along with a main() function providing a command-line interface.
See usage documentation in main(), below.
"""
from __future__ import print_function
import collections
import numpy as np
import vigra

def main():
    """
    Input:
        - A stack of superpixels in .png fromat (exported from Raveler),
            where each file is named yada-yada.NNNNN.png
        - A superpixel-to-segment mapping txt file,where each line is:
            <plane index> <superpixel id> <segment id>
        - A segment-to-body mapping txt file, where each line is:
            <segment id> <body id>
    
    Output:
        An hdf5 volume of all planes, remapped to use body IDs (uint64)
    
    TODO: It would be trivial to refactor the main loop in this script to support larger-than-RAM volumes.
    """
    import os
    import sys
    import re
    import argparse
    import h5py
    
    ## Debug example...
    #os.chdir('/groups/flyem/data/temp/anicetor/exports/fib19')
    #sys.argv += '--output-path=/tmp/output.h5 \
    #            superpixel_to_segment_map.txt \
    #            segment_to_body_map.txt \
    #            superpixel_maps/sp_map.00100.png \
    #            superpixel_maps/sp_map.00101.png'.split()
    
    parser = argparse.ArgumentParser()
    parser.add_argument("superpixel_to_segment_filepath")
    parser.add_argument("segment_to_body_filepath")
    parser.add_argument("image_paths", nargs="+")
    parser.add_argument("--output-path", required=False)
    parsed_args = parser.parse_args()
    
    if not parsed_args.output_path:
        input_name, ext = os.path.splitext(os.path.split(parsed_args.image_paths[0])[1])
        parsed_args.output_path = os.getcwd() + os.sep + input_name + '-bodies.h5'
    assert parsed_args.output_path[-3:] == '.h5', "Output path must end with .h5 extension."
    
    print("Parsing mappings...")
    plane_to_superpixel_to_segment_mapping = parse_superpixel_to_segment_file(parsed_args.superpixel_to_segment_filepath)
    segment_to_body_mapping = parse_segment_to_body_file(parsed_args.segment_to_body_filepath)

    # We assume superpixel pngs end with NNN.png, where NNN is the Z-index.
    index_rgx = re.compile('.*\D(\d+)\.png')

    body_imgs = []
    for image_path in parsed_args.image_paths:
        match = index_rgx.match(image_path)
        assert match, "Could not extract plane index from image path: {}".format(image_path)
        plane = int(match.groups()[0]) 

        print("Reading {}...".format( image_path ))
        superpixel_img = read_superpixel_png(image_path)
        body_img = relabel_superpixels_to_bodies( plane_to_superpixel_to_segment_mapping[plane],
                                                  segment_to_body_mapping,
                                                  superpixel_img )
        body_imgs.append( body_img[None] )

    print("Concatenating...")
    vol_3d = np.concatenate(body_imgs, axis=0)

    print("Writing to {}...".format( parsed_args.output_path ))
    with h5py.File(parsed_args.output_path, 'w') as f:
        f.create_dataset( 'bodies', 
                          data=vol_3d, 
                          chunks=True, 
                          compression='gzip', 
                          compression_opts=1 ) # Labels are highly compressible, 
                                               # so fastest option should be fine.
    print("DONE.")

def relabel_superpixels_to_bodies( superpixel_to_segment_mapping,
                                   segment_to_body_mapping,
                                   superpixel_label_img ):
    """
    Apply the two mappings to the given label image.
    """
    segment_label_img = relabel_img( superpixel_to_segment_mapping, superpixel_label_img, np.uint64 )
    body_label_img = relabel_img( segment_to_body_mapping, segment_label_img, np.uint64 )
    return body_label_img

def relabel_img(sorted_mapping, label_img, output_dtype=None):
    """
    Given a sorted OrderedDict of input labels -> output labels,
    relabel the given input image according to the mapping.
    """
    assert isinstance(sorted_mapping, collections.OrderedDict)
    output_dtype = output_dtype or label_img.dtype
    indexed_data = np.searchsorted(sorted_mapping.keys(), label_img)
    relabeled_data = np.array(sorted_mapping.values(), dtype=output_dtype)[indexed_data]
    return relabeled_data

def parse_superpixel_to_segment_file(path):
    """
    Read the superpixel-to-segment text file and return a sorted dict-of-dicts:
        mapping[plane][superpixel_id] -> segment_id
    """
    mappings = collections.defaultdict(lambda: {})
    with open(path, 'r') as f:
        for line in f:
            plane, superpixel, segment = map(int, line.split())
            mappings[plane][superpixel] = segment    
    sorted_mappings = collections.OrderedDict()
    for plane in sorted( mappings.keys() ):
        sorted_mappings[plane] = collections.OrderedDict( sorted( mappings[plane].items() ) )
    return sorted_mappings

def parse_segment_to_body_file(path):
    """
    Read the segment-to-body text file and return a sorted dict:
        mapping[segment_id] -> body_id
    """
    mapping = {}
    with open(path, 'r') as f:
        for line in f:
            segment, body = map(int, line.split())
            mapping[segment] = body

    sorted_mapping = collections.OrderedDict( sorted( mapping.items() ) )
    return sorted_mapping

def read_superpixel_png(png_path):
    """
    Read the given png and return a uint32 image of superpixel ids.
    Raveler encodes 24-bit superpixel IDs into the RGB channels of a png image,
    so we simply decode it as a uint32 value (after discarding the Alpha channel).
    (Raveler sets the alpha channel to 255)
    """
    png_data = vigra.impex.readImage(image_path, dtype='NATIVE').withAxes('yxc')
    assert png_data.ndim == 3
    assert png_data.shape[-1] == 4
    assert png_data.flags['C_CONTIGUOUS']
    
    # Zero-out the alpha channel
    png_data[...,-1] = 0
    return png_data.view(np.uint32)

if __name__ == "__main__":
    sys.exit( main() )
