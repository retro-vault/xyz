const vscode = require("vscode");

class xgdb_configuration_provider {
    resolveDebugConfiguration(workspace_folder, config) {
        const resolved_config = { ...config };

        if (!resolved_config.type && !resolved_config.request && !resolved_config.name) {
            resolved_config.name = "xgdb launch";
            resolved_config.type = "xgdb";
            resolved_config.request = "launch";
        }

        resolved_config.type = "xgdb";
        resolved_config.request = resolved_config.request || "launch";
        resolved_config.name = resolved_config.name || "xgdb launch";
        resolved_config.debuggerPath = resolved_config.debuggerPath || "xgdb";
        resolved_config.cwd = resolved_config.cwd || infer_cwd(workspace_folder);

        if (resolved_config.stopOnEntry === undefined) {
            resolved_config.stopOnEntry = true;
        }

        if (!resolved_config.program) {
            return vscode.window.showErrorMessage(
                "xgdb launch configuration requires a program path."
            ).then(() => undefined);
        }

        if (!resolved_config.symbols) {
            return vscode.window.showErrorMessage(
                "xgdb launch configuration requires a symbols path."
            ).then(() => undefined);
        }

        if (!resolved_config.remoteTarget) {
            return vscode.window.showErrorMessage(
                "xgdb launch configuration requires a remoteTarget value."
            ).then(() => undefined);
        }

        return resolved_config;
    }
}

class xgdb_adapter_factory {
    createDebugAdapterDescriptor(session) {
        const debugger_path = session.configuration.debuggerPath || "xgdb";
        const adapter_cwd =
            session.configuration.cwd ||
            infer_cwd(session.workspaceFolder) ||
            undefined;

        return new vscode.DebugAdapterExecutable(
            debugger_path,
            ["--interpreter=dap", "--quiet"],
            adapter_cwd ? { cwd: adapter_cwd } : undefined
        );
    }
}

function infer_cwd(workspace_folder) {
    if (!workspace_folder || !workspace_folder.uri) {
        return process.cwd();
    }
    return workspace_folder.uri.fsPath;
}

function activate(context) {
    const configuration_provider = new xgdb_configuration_provider();
    const adapter_factory = new xgdb_adapter_factory();

    context.subscriptions.push(
        vscode.debug.registerDebugConfigurationProvider("xgdb", configuration_provider)
    );
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory("xgdb", adapter_factory)
    );
}

function deactivate() {
}

module.exports = {
    activate,
    deactivate
};
