// Update year
const yearEl = document.getElementById('year');
if(yearEl) yearEl.textContent = new Date().getFullYear();

// Simple reveal-on-scroll
const io = new IntersectionObserver((entries)=>{
  entries.forEach(e=>{ if(e.isIntersecting){ e.target.classList.add('is-visible'); io.unobserve(e.target); } });
},{threshold:0.12});
document.querySelectorAll('.card, .section-head, .profile-link, .hero-left').forEach(n=>io.observe(n));

// small hover parallax (non-essential)
document.addEventListener('pointermove', (ev)=>{
  const x = (ev.clientX / window.innerWidth) - 0.5;
  const y = (ev.clientY / window.innerHeight) - 0.5;
  document.documentElement.style.setProperty('--parallax-x', (x*8)+'px');
  document.documentElement.style.setProperty('--parallax-y', (y*8)+'px');
});
// (Theme toggle removed — site is dark-only.)

// Mobile app screenshot lightbox
;(function(){
  const modal = document.createElement('div');
  modal.id = 'app-modal';
  modal.className = 'app-modal';
  modal.innerHTML = `
    <button class="modal-close" aria-label="Close">×</button>
    <img class="modal-image" src="" alt="">
    <div class="modal-caption"></div>
  `;
  document.body.appendChild(modal);

  const modalImage = modal.querySelector('.modal-image');
  const modalCaption = modal.querySelector('.modal-caption');
  const closeBtn = modal.querySelector('.modal-close');

  function openModal(src, caption){
    modalImage.src = src;
    modalImage.alt = caption || 'Mobile screenshot';
    modalCaption.textContent = caption || '';
    modal.classList.add('is-open');
    modal.setAttribute('aria-hidden','false');
  }
  function closeModal(){
    modal.classList.remove('is-open');
    modal.setAttribute('aria-hidden','true');
    modalImage.src = '';
  }

  document.addEventListener('click', (e)=>{
    const t = e.target.closest && e.target.closest('[data-screenshot]');
    if(t){
      const src = t.getAttribute('data-screenshot') || t.src;
      const caption = t.closest('figure')?.querySelector('figcaption')?.textContent || '';
      openModal(src, caption);
    }
    if(e.target === closeBtn || e.target === modal) closeModal();
  });

  document.addEventListener('keydown', (e)=>{ if(e.key === 'Escape') closeModal(); });
})();