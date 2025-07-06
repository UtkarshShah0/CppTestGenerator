import os
import glob
from collections import defaultdict
from azure.ai.inference import ChatCompletionsClient
from azure.ai.inference.models import SystemMessage, UserMessage
from azure.core.credentials import AzureKeyCredential
from dotenv import load_dotenv

load_dotenv()

ENDPOINT = "https://models.github.ai/inference"
MODEL = "xai/grok-3"
TOKEN = os.environ["GITHUB_TOKEN"]
ORGCHART_ROOT = "../orgChartApi"
TESTS_DIR = "../orgChartApi/generated_tests"

def read_yaml_prompt(filename):
    with open(os.path.join("../testGenerator/prompts", filename), "r") as f:
        return f.read()

def call_grok_llm(prompt, code):
    print(f"Calling Grok-3 for code block of length {len(code)}...")
    client = ChatCompletionsClient(
        endpoint=ENDPOINT,
        credential=AzureKeyCredential(TOKEN),
    )
    response = client.complete(
        messages=[
            SystemMessage("You are an expert C++ developer and test writer."),
            UserMessage(f"{prompt}\n\n---\n\n{code}\n\n---\n\n# Output only the test code.")
        ],
        temperature=0.2,
        top_p=1.0,
        model=MODEL
    )
    print("Grok-3 call successful.")
    return response.choices[0].message.content

def get_file_groups(target_dir):
    # Group files by base name (without extension)
    files = []
    for ext in ("*.cpp", "*.cc", "*.h", "*.hpp"):
        files.extend(glob.glob(os.path.join(target_dir, ext)))
    groups = defaultdict(list)
    for f in files:
        base = os.path.splitext(os.path.basename(f))[0]
        groups[base].append(f)
    print(f"Found {len(groups)} unique file(s) in {target_dir}: {list(groups.keys())}")
    return groups

def write_test_file(base_name, test_code):
    test_file = os.path.join(TESTS_DIR, f"test_{base_name}.cpp")
    with open(test_file, "w", encoding="utf-8") as f:
        f.write(test_code)
    print(f"Test file written: {test_file}")
    return test_file

def generate_tests_for_dir(target_dir, prompt_file, label):
    print(f"\n[Step] Generating unit tests for {label}...")
    prompt = read_yaml_prompt(prompt_file)
    groups = get_file_groups(target_dir)
    for base_name, file_list in groups.items():
        print(f"Generating test for: {base_name} (from {file_list})")
        code = ""
        for f in file_list:
            with open(f, "r", encoding="utf-8", errors="replace") as file:
                code += f"\n// File: {os.path.basename(f)}\n"
                code += file.read()
        test_code = call_grok_llm(prompt, code)
        write_test_file(base_name, test_code)
    print(f"[Step] Test generation for {label} complete.\n")

def main():
    print("=== Unit Test Generator: Controllers, Models, Utils (Grok-3, grouped) ===")
    os.makedirs(TESTS_DIR, exist_ok=True)

    # You can change the prompt file as needed for each type
    generate_tests_for_dir(
        target_dir=os.path.join(ORGCHART_ROOT, "controllers"),
        prompt_file="initial.yaml",
        label="controllers"
    )
    generate_tests_for_dir(
        target_dir=os.path.join(ORGCHART_ROOT, "models"),
        prompt_file="initial.yaml",
        label="models"
    )
    generate_tests_for_dir(
        target_dir=os.path.join(ORGCHART_ROOT, "utils"),
        prompt_file="initial.yaml",
        label="utils"
    )

if __name__ == "__main__":
    main()