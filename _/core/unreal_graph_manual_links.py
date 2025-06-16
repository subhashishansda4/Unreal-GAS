import os
import re
import json

# Configure these paths to your project
SOURCE_DIR = r"C:\Users\Rahul\Documents\Unreal Projects\Aura\Source\Aura"
BLUEPRINT_DIR = r"C:\Users\Rahul\Documents\Unreal Projects\Aura\Content\Blueprints"

# Regex patterns for C++ parsing
INCLUDE_PATTERN = re.compile(r'#include\s+"([^"]+)"')
CLASS_PATTERN = re.compile(r'^\s*(?:UCLASS\(.*?\))?\s*class\s+(\w+)(?:\s*:\s*public\s+([\w:]+))?', re.MULTILINE)
UCLASS_PATTERN = re.compile(r'UCLASS\((.*?)\)')
UPROPERTY_PATTERN = re.compile(r'UPROPERTY\((.*?)\)\s*(?:[\w\s]*)(\w+)\s*;')
UFUNCTION_PATTERN = re.compile(r'UFUNCTION\((.*?)\)\s*([\w\s]+)\(')

def parse_cpp_file(filepath):
    data = {
        "includes": [],
        "classes": []
    }
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        # Extract includes
        data["includes"] = INCLUDE_PATTERN.findall(content)

        # Extract classes
        classes = []
        for match in CLASS_PATTERN.finditer(content):
            class_name = match.group(1)
            base_class = match.group(2) if match.group(2) else None

            # Find if UCLASS exists before this class (search in preceding lines)
            start_pos = match.start()
            before_class = content[max(0, start_pos-300):start_pos]  # check 300 chars before
            uclass_match = UCLASS_PATTERN.search(before_class)
            uclass = bool(uclass_match)

            # Find UPROPERTY and UFUNCTIONs within this class - crude approach:
            # We'll just collect all UPROPERTYs and UFUNCTIONs in whole file for simplicity
            # (Could be enhanced with better parsing)
            uproperties = [{"specifiers": m.group(1), "name": m.group(2)} for m in UPROPERTY_PATTERN.finditer(content)]
            ufunctions = [{"specifiers": m.group(1), "declaration": m.group(2).strip()} for m in UFUNCTION_PATTERN.finditer(content)]

            classes.append({
                "name": class_name,
                "inherits": [base_class] if base_class else [],
                "uclass": uclass,
                "uproperties": uproperties,
                "ufunctions": ufunctions,
            })
            break  # For now just first class per file to avoid noise (optional to remove this)

        data["classes"] = classes

    except Exception as e:
        print(f"Failed to parse C++ file {filepath}: {e}")

    return data

def scan_cpp_files(source_dir):
    cpp_data = {}
    for root, dirs, files in os.walk(source_dir):
        for f in files:
            if f.endswith(('.h', '.cpp')):
                full_path = os.path.join(root, f)
                rel_path = os.path.relpath(full_path, source_dir)
                cpp_data[rel_path.replace('\\','/')] = parse_cpp_file(full_path)
    return cpp_data

# Blueprint parsing based on folder + filename only (no .uasset parse)
def scan_blueprints(blueprint_dir):
    bp_data = {}

    def guess_type_from_path(path):
        lower_path = path.lower()
        # Basic heuristics from your structure and filenames
        if "ability" in lower_path:
            return "gameplayability"
        if "effect" in lower_path or "gameplayeffect" in lower_path:
            return "gameplayeffect"
        if "cue" in lower_path or "gameplaycue" in lower_path:
            return "gameplaycue"
        if "datatable" in lower_path or path.endswith('.csv') or path.endswith('.json'):
            return "datatable"
        if "dataasset" in lower_path:
            return "dataasset"
        if "curvetable" in lower_path:
            return "curvetable"
        if "blueprint" in lower_path or path.endswith(".uasset"):
            return "blueprint"
        if path.endswith(".umap"):
            return "map"
        return "unknown"

    for root, dirs, files in os.walk(blueprint_dir):
        for f in files:
            if not f.lower().endswith(('.uasset', '.umap', '.json', '.csv')):
                continue  # Skip files that are not UE assets or data tables

            full_path = os.path.join(root, f)
            rel_path = os.path.relpath(full_path, blueprint_dir).replace('\\','/')

            bp_type = guess_type_from_path(rel_path)

            # We won't parse inside .uasset since it's binary,
            # but we can guess dependencies by naming conventions or directory relations
            # For simplicity, dependencies list will be empty here
            bp_data[rel_path] = {
                "type": bp_type,
                "dependencies": []  # could be extended with better heuristics
            }

    return bp_data

def generate_report():
    print("Scanning C++ files...")
    cpp_files = scan_cpp_files(SOURCE_DIR)
    print(f"Parsed {len(cpp_files)} C++ files.")

    print("Scanning Blueprints and assets...")
    blueprints = scan_blueprints(BLUEPRINT_DIR)
    print(f"Discovered {len(blueprints)} blueprint/assets files.")

    report = {
        "cpp_files": cpp_files,
        "blueprints": blueprints
    }

    return report

def main():
    report = generate_report()
    with open("unreal_project_dependency_report.json", "w", encoding='utf-8') as f:
        json.dump(report, f, indent=4)
    print("Report saved to unreal_project_dependency_report.json")

if __name__ == "__main__":
    main()
