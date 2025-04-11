/************************************************************************/
/**
 * @file chFileSystem.h
 * @author AccelMR
 * @date 2022/06/27
 * @brief File system that is platform specific.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chFileStream.h"

namespace chEngineSDK{
class CH_UTILITY_EXPORT FileSystem {
 public:

 /** 
  *   Creates a directory with a given path.
  *  NOTE: Path must end at '/' to be a directory.
  * 
  * @param path
  *   The path to create thee directory. Has to end with '/'
  * 
  * @return bool
  *   True if directory could be created.
  **/
  static bool
  createDirectory(const Path& path);

  /** 
   *   Creates directories and its parents if they don't exist.
  *  NOTE: Path must end at '/' to be a directory.
  *
  * @param path
  *   The path to create thee directory. Has to end with '/'
  *
  * @return bool
  *   True if directories could be created.
   **/
  static bool
  createDirectories(const Path& path);

  /** 
   *   Returns true if a path exist, doesn't matter if it is file or directory.
   **/
  static bool
  exists(const Path& path);

  /** 
   *   Opens a file and positions the pointer to the end of the file.
   * 
   * @param path
   *    The path where the file is.
   * 
   * @param readOnly = true
   *    Tue if this file will be only for reading.
   * 
   * @return SPtr<DataStream>
   *  Shared pointer to the Data stream related with this Stream.
   **/
  static SPtr<DataStream>
  openFile(const Path& path, bool readOnly = true);

  /** 
   *   Creates and opens a file. If file exist this will override all information.
   *   If the directory doesn't exist, it will be created.
   * 
   * @param path
   *    The path where the file is going to be located.
   * 
   * @return SPtr<DataStream>
   *  Pointer to the created file, nullptr if could not be created.
   **/
  static SPtr<DataStream>
  createAndOpenFile(const Path& path);

  /** 
   *   Dumps the information from a MemoryDataStream into a FileStream.
   * 
   * @param memStream
   *    Source to take data.
   * 
   * @param path
   *    Where to dumps the data.
   **/
  static void
  dumpMemStreamIntoFile(const SPtr<DataStream>& memStream, const Path& path);

  /** 
   *   Removes a file or empty directory.
   * 
   * NOTE: Will throw an exception if the directory is not empty.
   * 
   * @param path
   *    What to delete.
   * 
   * @return bool
   *    True if it could be deleted.
   **/
  static bool
  remove(const Path& path);
  
  /** 
   *   
   * Removes all data in path, ignoring if it has data in it or not.
   *
   * @param path
   *    What to delete.
   * 
   * @return bool
   *    True if it could be deleted.
   **/
  static bool
  removeAll(const Path& path);

  /** 
   *   Reads a file and returns it as a byte Vector.
   * 
   * @param path
   *  The path where the file is.
   * 
   * @return Vector<uint8*>
   *  Byte array of the file.
   **/
  static Vector<uint8>
  fastRead(const Path& path);

  /*
  */
  static bool 
  isPathRelative(const Path& baseTarget, const Path& target);

  static void 
  getChildren(const Path& path, Vector<Path>& files, Vector<Path>& directories);

};
}
