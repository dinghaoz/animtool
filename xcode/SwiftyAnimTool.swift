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

func DropFrames(
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
