// Wait for the DOM to be fully loaded before running scripts
document.addEventListener('DOMContentLoaded', () => {

    // --- 1. Create Quiz Page Logic ---
    const createQuizPage = document.querySelector('.create-quiz-container');
    if (createQuizPage) {
        const questionsContainer = document.getElementById('questions-container');
        const addQuestionBtn = document.getElementById('add-question-btn');
        let questionCount = 0;

        const addQuestion = () => {
            questionCount++;
            const questionCard = document.createElement('div');
            questionCard.className = 'card quiz-question-builder';
            questionCard.innerHTML = `
                <div class="question-header">
                    <h4>Question ${questionCount}</h4>
                    <button type="button" class="btn-icon remove-question-btn"><i class="fas fa-trash"></i></button>
                </div>
                <div class="input-group">
                    <i class="fas fa-question-circle"></i>
                    <input type="text" name="q${questionCount}_text" placeholder="Enter the question text" required>
                </div>
                <div class="input-group">
                    <i class="fas fa-check-circle" style="color: var(--success-color);"></i>
                    <input type="text" name="q${questionCount}_opt1" placeholder="Correct Answer" required>
                </div>
                <div class="input-group">
                    <i class="fas fa-times-circle"></i>
                    <input type="text" name="q${questionCount}_opt2" placeholder="Incorrect Option 2" required>
                </div>
                <div class="input-group">
                    <i class="fas fa-times-circle"></i>
                    <input type="text" name="q${questionCount}_opt3" placeholder="Incorrect Option 3" required>
                </div>
                <div class="input-group">
                    <i class="fas fa-times-circle"></i>
                    <input type="text" name="q${questionCount}_opt4" placeholder="Incorrect Option 4" required>
                </div>
            `;
            questionsContainer.appendChild(questionCard);

            // Add event listener to the new remove button
            questionCard.querySelector('.remove-question-btn').addEventListener('click', () => {
                questionCard.remove();
                // Note: Re-numbering questions after removal can be complex,
                // so the backend should handle non-sequential numbers (q1, q3, q4...).
            });
        };

        // Add the first question by default
        addQuestion();
        
        // Add another question when the button is clicked
        addQuestionBtn.addEventListener('click', addQuestion);
    }

    // --- 2. Quiz Attempt Page Logic ---
    const quizAttemptPage = document.querySelector('.quiz-attempt-container');
    if (quizAttemptPage) {
        const options = document.querySelectorAll('.option');
        options.forEach(option => {
            option.addEventListener('click', () => {
                // Get the name of the radio button group for the current question
                const radioName = option.querySelector('input[type="radio"]').name;
                
                // Remove 'selected' class from all options in the same question group
                document.querySelectorAll(input[name="${radioName}"]).forEach(radio => {
                    radio.parentElement.classList.remove('selected');
                });
                
                // Add 'selected' class to the clicked option and check the radio button
                option.classList.add('selected');
                option.querySelector('input[type="radio"]').checked = true;
            });
        });
    }

    // --- 3. Dashboard Profile Dropdown Logic ---
    const profile = document.querySelector('.profile');
    if (profile) {
        profile.addEventListener('click', () => {
            const dropdown = profile.querySelector('.profile-dropdown');
            dropdown.style.display = dropdown.style.display === 'block' ? 'none' : 'block';
        });

        // Hide dropdown if clicked outside
        window.addEventListener('click', (event) => {
            if (!profile.contains(event.target)) {
                profile.querySelector('.profile-dropdown').style.display = 'none';
            }
        });
    }

});