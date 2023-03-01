//
//  animtool.swift
//  animtool_ios
//
//  Created by Dinghao Zeng on 2023/2/24.
//

import Foundation

public enum AnimToolError: Error {
  case failed
  case invalidArgs
}

public func DropFrames(
  input: String,
  output: String,
  targetFrameRate: Int,
  targetTotalDuration: Int,
  loopCount: Int,
  srcRect: CGRect,
  dstSize: CGSize
) throws {
  if AnimToolDropFramesLite(
    input.cString(using: .utf8),
    output.cString(using: .utf8),
    Int32(targetFrameRate),
    Int32(targetTotalDuration),
    Int32(loopCount),
    Int32(srcRect.origin.x),
    Int32(srcRect.origin.y),
    Int32(srcRect.size.width),
    Int32(srcRect.size.height),
    Int32(dstSize.width),
    Int32(dstSize.height)
  ) == 0 {
    throw AnimToolError.failed
  }
}

public func Animate(
  inputs: [String],
  duration: Int,
  dstSize: CGSize,
  output: String
) throws {
  let paths = inputs.map {
    UnsafePointer(strdup($0))
  }

  defer {
    paths.forEach {
      free(UnsafeMutablePointer(mutating: $0))
    }
  }

  if AnimToolAnimateLite(
    paths,
    Int32(inputs.count),
    Int32(duration),
    Int32(dstSize.width),
    Int32(dstSize.height)
    output.cString(using: .utf8)
  ) == 0 {
    throw AnimToolError.failed
  }
}
