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

public enum Background {
  case file(String)
  case color(String)
  case frame(Int)

  func toStr() -> String {
    switch self {
    case .file(let f): return "file\(f)"
    case .color(let c): return "color:\(c)"
    case .frame(let i): return "frame:\(i)"
    }
  }
}

public func Animate(
  inputs: [String],
  background: Background,
  backgroundBlurRadius: Int,
  canvasSize: CGSize,
  duration: Int,
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
    background.toStr().cString(using: .utf8),
    Int32(backgroundBlurRadius),
    Int32(canvasSize.width),
    Int32(canvasSize.height),
    Int32(duration),
    output.cString(using: .utf8)
  ) == 0 {
    throw AnimToolError.failed
  }
}
