from grader import Grader
import math

class testGrader(Grader):
    def grade(self, grader_path, grader_config, student_response):
        wrong_result = {
            'score': 0,
            'msg': "Something is incorrect, try again!",
        }
        correct_result = {
            'score': 1,
            'msg': "Good job!",
        }

        student_res = str(student_response)
        res = str(math.factorial(5))

        if student_res == res:
            return correct_result
        return wrong_result
