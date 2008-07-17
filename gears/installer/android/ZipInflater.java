// Copyright 2008, Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.android.gears;

import android.util.Log;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;


/**
 * A class that can inflate a zip archive.
 */
public final class ZipInflater {
  /**
   * Logging tag
   */
  private static final String TAG = "Gears-J-ZipInflater";

  /**
   * The size of the buffer used to read from the archive.
   */
  private static final int BUFFER_SIZE_BYTES = 32 * 1024;  // 32 KB.

  private static final String PATH_NAVIGATION_COMPONENT = ".." + File.separator;

  /**
   * Unzips the archive with the given name.
   * @param filename is the name of the zip to inflate.
   * @param path is the path where the zip should be unpacked. It must contain
   *             a trailing separator, or the extraction will fail.
   * @return true if the extraction is successful and false otherwise.
   */
  public static boolean inflate(String filename, String path) {
     try {

       // Check that the path ends with a separator.
       if (!path.endsWith(File.separator)) {
         throw new IOException("Path missing trailing separator: " + path);
       }

       ZipInputStream zip = new ZipInputStream(
           new BufferedInputStream(new FileInputStream(filename)));
       ZipEntry entry;
       int counter;
       byte buffer[] = new byte[BUFFER_SIZE_BYTES];

       // Iterate through the entries and write each of them to a file.
       while ((entry = zip.getNextEntry()) != null) {
         // Check against entry names that may attempt to navigate
         // out of the destination directory.
         if (entry.getName().indexOf(PATH_NAVIGATION_COMPONENT) >= 0) {
           throw new IOException("Illegal entry name: " + entry.getName());
         }

         // Check against entries with the same name as pre-existing files or
         // directories.
         File file = new File(path + entry.getName());
         if (file.exists()) {
           // A file or directory with the same name already exist.
           // This must not happen, so we treat this as an error.
           throw new IOException(
               "A file or directory with the same name already exists.");
         }

         if (entry.isDirectory()) {
           // If the entry denotes a directory, we need to create a
           // directory with the same name.
           file.mkdirs();
         } else {
           CRC32 checksum = new CRC32();
           BufferedOutputStream output = new BufferedOutputStream(
               new FileOutputStream(file),
               BUFFER_SIZE_BYTES);
           // Read the entry and write it to the file.
           while ((counter = zip.read(buffer, 0, BUFFER_SIZE_BYTES)) != -1) {
             output.write(buffer, 0, counter);
             checksum.update(buffer, 0, counter);
           }

           output.flush();
           output.close();

           if (checksum.getValue() != entry.getCrc()) {
             throw new IOException(
                 "Integrity check failed for: " + entry.getName());
           }
         }
       }

       zip.close();
       return true;

     } catch (FileNotFoundException ex) {
       Log.e(TAG, "The zip file could not be found. " + ex);
       return false;
     } catch (IOException ex) {
       Log.e(TAG, "Could read or write an entry. " + ex);
       return false;
     } catch(IllegalArgumentException ex) {
       Log.e(TAG, "Could not create the BufferedOutputStream. " + ex);
       return false;
     }
  }
}
