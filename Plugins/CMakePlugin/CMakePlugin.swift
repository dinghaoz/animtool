import PackagePlugin
import Foundation

@main
struct CMakePlugin: BuildToolPlugin {
    func createBuildCommands(context: PackagePlugin.PluginContext, target: PackagePlugin.Target) async throws -> [PackagePlugin.Command] {
        let output = context.pluginWorkDirectory.appending("build.ios")
        return [
            .prebuildCommand(displayName: "CMake generate Xcode project",
                executable: context.package.directory.appending("buildios.sh"),
                arguments: [context.package.directory, output],
                environment: [:],
                outputFilesDirectory:output)
        ]
    }
//     func performCommand(context: PluginContext, arguments: [String]) throws {
//         let tool = try context.tool(named: "swiftlint")
//         let toolUrl = URL(fileURLWithPath: tool.path.string)
//
//         for target in context.package.targets {
//             guard let target = target as? SourceModuleTarget else { continue }
//
//             let process = Process()
//             process.executableURL = toolUrl
//             process.arguments = [
//                 "\(target.directory)",
//                 "--fix",
//                // "--in-process-sourcekit" // this line will fix the issues...
//             ]
//
//             try process.run()
//             process.waitUntilExit()
//
//             if process.terminationReason == .exit && process.terminationStatus == 0 {
//                 print("Formatted the source code in \(target.directory).")
//             }
//             else {
//                 let problem = "\(process.terminationReason):\(process.terminationStatus)"
//                 Diagnostics.error("swift-format invocation failed: \(problem)")
//             }
//         }
//     }
}