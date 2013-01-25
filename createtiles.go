// Copyright 2013 HHMI.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of HHMI nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: katzw@janelia.hhmi.org (Bill Katz)
//  Written as part of the FlyEM Project at Janelia Farm Research Center.

package main

import (
	"flag"
	"fmt"
	"image"
	_ "image/png"
	"log"
	"os"
	"path/filepath"
	"strings"
)

var showHelp bool
var showHelp2 bool

func inspectImage(filename string) (width, height int) {
	file, err := os.Open(filename)
	if err != nil {
		log.Fatal("FATAL ERROR: opening ", filename, ": ", err)
	}

	grayscale, _, err := image.Decode(file)
	if err != nil {
		log.Fatal("FATAL ERROR: decoding ", filename, ": ", err)
	}
	width = grayscale.Bounds().Dx()
	height = grayscale.Bounds().Dy()

	file.Close()
	return
}

func makeTileName(dirName, fname string) (tileName string, z int) {
	parts := strings.Split(fname, ".")
	_, err := fmt.Sscanf(parts[len(parts)-2], "%05d", &z)
	if err != nil {
		log.Fatalf("Error parsing filename %s [%s]\n", fname, err)
	}
	tileName = filepath.Join(dirName, fmt.Sprintf("%03d.png", z))
	return
}

func writeMetadata(file *os.File, width, height, zmin, zmax int) {
	metadata := []string{
		"version=1",
		fmt.Sprintf("width=%d", width),
		fmt.Sprintf("height=%d", height),
		fmt.Sprintf("zmin=%d", zmin),
		fmt.Sprintf("zmax=%d", zmax),
		`channels="g;s"`,
		"superpixel-format=I"}
	_, err := fmt.Fprintln(file, strings.Join(metadata, "\n"))
	if err != nil {
		log.Fatalf("ERROR: writing metadata [%s]\n", err)
	}
}

func init() {
	flag.BoolVar(&showHelp, "h", false, "Show help message")
	flag.BoolVar(&showHelp2, "help", false, "Show help message")
}

func main() {
	programName := filepath.Base(os.Args[0])
	flag.Parse()
	if flag.NArg() != 3 || showHelp || showHelp2 {
		fmt.Println(programName, "creates a Raveler-ready tiles directory for grayscale",
			"and superpixel png images no larger than 999 x 999.\n")
		fmt.Println("usage:", programName, "[options] <grayscale dir>",
			"<superpixel dir> <output dir>\n")
		flag.PrintDefaults()
		os.Exit(1)
	}

	grayscaleDir := flag.Arg(0)
	superpixelDir := flag.Arg(1)
	outputDir := flag.Arg(2)

	// Get all grayscale and superpixel image filenames.
	grayscaleFnames, err := filepath.Glob(filepath.Join(grayscaleDir, "*[0123456789].png"))
	if err != nil {
		log.Fatalf("ERROR: Could not read files in grayscale directory: %s [%s]\n",
			grayscaleDir, err)
	}
	fmt.Printf("Detected %d grayscale images in directory: %s\n",
		len(grayscaleFnames), grayscaleDir)
	if len(grayscaleFnames) == 0 {
		log.Fatalln("Aborting...")
	}
	superpixelFnames, err := filepath.Glob(filepath.Join(superpixelDir, "*[0123456789].png"))
	if err != nil {
		log.Fatalf("ERROR: Could not read files in superpixel directory: %s [%s]\n",
			superpixelDir, err)
	}
	fmt.Printf("Detected %d superpixel images in directory: %s\n",
		len(superpixelFnames), superpixelDir)

	// Get the image size
	width, height := inspectImage(grayscaleFnames[0])

	// Make sure the output directory has been created
	err = os.Mkdir(outputDir, 0755)
	if err != nil {
		if !os.IsExist(err) {
			log.Fatalln("ERROR: creating output directory:", outputDir)
		}
	}

	// Copy grayscale files
	grayscaleTileDir := filepath.Join(outputDir, "1024", "0", "0", "0", "g")
	err = os.MkdirAll(grayscaleTileDir, 0755)
	if err != nil {
		if !os.IsExist(err) {
			log.Fatalln("ERROR: creating grayscale tile directory:", grayscaleTileDir)
		}
	}
	zmin := -1
	zmax := -1
	for _, fname := range grayscaleFnames {
		newname, z := makeTileName(grayscaleTileDir, fname)
		if zmin < 0 || zmin > z {
			zmin = z
		}
		if zmax < 0 || zmax < z {
			zmax = z
		}
		err = os.Symlink(fname, newname)
		if err != nil {
			log.Fatalf("ERROR: Could not create symbolic link %s -> %s\n", fname, newname)
		}
	}

	// Copy superpixel files
	superpixelTileDir := filepath.Join(outputDir, "1024", "0", "0", "0", "s")
	err = os.MkdirAll(superpixelTileDir, 0755)
	if err != nil {
		if !os.IsExist(err) {
			log.Fatalln("ERROR: creating superpixel tile directory:", superpixelTileDir)
		}
	}
	for _, fname := range superpixelFnames {
		newname, _ := makeTileName(superpixelTileDir, fname)
		err = os.Symlink(fname, newname)
		if err != nil {
			log.Fatalf("ERROR: Could not create symbolic link %s -> %s\n", fname, newname)
		}
	}

	// Create the metadata.txt file
	metadataFname := filepath.Join(outputDir, "metadata.txt")
	file, err := os.Create(metadataFname)
	if err != nil {
		log.Fatalf("ERROR: trying to create metadata file: %s [%s]\n",
			metadataFname, err)
	}
	writeMetadata(file, width, height, zmin, zmax)
	file.Close()
}
