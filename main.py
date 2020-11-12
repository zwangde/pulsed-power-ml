import luigi

from src.data.Task1 import Task1
from src.data.Task2 import Task2

class ExampleWrapperTask(luigi.WrapperTask):
    """Run workflow to create word count and feature files

    """

    def requires(self):
        """Requires method for Luigi

        https://luigi.readthedocs.io/en/stable/tasks.html
        """

        return [Task1(), Task2()]


if __name__ == '__main__':
    luigi.build([ExampleWrapperTask()],
                local_scheduler=False, detailed_summary=True, workers=7)
