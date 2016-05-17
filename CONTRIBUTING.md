# Contributing to Anvil

Thank you for your interest in Anvil.

Contributions can be made by creating a GitHub pull request.

---

**IMPORTANT**: By creating a pull request, you agree to allow your contribution to be licensed by the project owners under the terms of the [MIT License](LICENSE.txt).

---

### Implementation guidelines
When contributing to Anvil, we mostly just want your code to be:

1. Cross-platform
2. Mindful of performance
3. Written in the style of the existing code

Here are a few specific guidelines:

* **Match the style of existing code**
  * This is the most important guideline
* Avoid adding new dependencies

### Commit messages
Please follow the standard Git conventions for commit messages.

Here is our summary of these conventions, adapted somewhat for GitHub's particular rules:

* Write a subject line followed by a blank line followed by further explanation (if needed)
* Capitalize the subject line
* Do not put a period at the end of the subject line
* Limit the subject line to 72 characters
  * A common recommendation is to limit it to 50 characters
  * GitHub truncates subject lines longer than 72 characters
* Use the imperative present tense
  * e.g. "Fix rendering bug"
  * Not "Fixed rendering bug", "Fixes rendering bug", or "Fixing rendering bug"
* Wrap the body (i.e. the "further explanation" part) at 72 characters

If you are using GitHub Desktop, the blank line and text wrapping will be handled for you.

More information on the standard conventions for good commit messages can be found here:

* GitHub blog entry on [Shiny new commit styles](https://github.com/blog/926-shiny-new-commit-styles)
* Chris Beams' post on [How to Write a Git Commit Message](http://chris.beams.io/posts/git-commit/)
* Tim Pope's original [Note About Git Commit Messages](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html)

In addition, if the commit fixes an open issue, add `Fixes issue_number` to the end of the commit message:

* e.g. `Fixes #1`
* See [Closing issues via commit messages](https://help.github.com/articles/closing-issues-via-commit-messages/) for more details
