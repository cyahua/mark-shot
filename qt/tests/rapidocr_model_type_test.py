#!/usr/bin/env python3
"""验证 rapidocr 版本和模型类型的合法组合。"""

import unittest
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = PROJECT_ROOT / "scripts" / "mark-shot-ocr"
LOADER = SourceFileLoader("mark_shot_ocr_model_type", str(SCRIPT_PATH))
SPEC = spec_from_loader(LOADER.name, LOADER)
OCR = module_from_spec(SPEC)
LOADER.exec_module(OCR)


class RapidOcrModelTypeTest(unittest.TestCase):
    def test_v5_does_not_keep_the_library_small_default(self):
        self.assertEqual(OCR.pick_model_type("PP-OCRv5", "Det", ""), "mobile")
        self.assertEqual(OCR.pick_model_type("PP-OCRv5", "Rec", "small"), "mobile")
        self.assertEqual(OCR.pick_model_type("PP-OCRv5", "Cls", "server"), "server")

    def test_v4_classification_has_no_server_route(self):
        self.assertEqual(OCR.pick_model_type("PP-OCRv4", "Cls", "server"), "mobile")

    def test_v6_keeps_small_and_skips_classification(self):
        self.assertEqual(OCR.pick_model_type("PP-OCRv6", "Det", "small"), "small")
        self.assertNotIn("Cls", OCR.TASK_MODEL_TYPES["PP-OCRv6"])

    def test_requested_type_wins_when_the_version_allows_it(self):
        self.assertEqual(OCR.pick_model_type("PP-OCRv5", "Rec", "server"), "server")


if __name__ == "__main__":
    unittest.main()
